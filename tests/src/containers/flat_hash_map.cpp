#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <neutron/flat_hash_map.hpp>
#include <neutron/utility.hpp>
#include "require.hpp"

namespace {

using int_map    = neutron::flat_hash_map<int, int>;
using string_map = neutron::flat_hash_map<std::string, std::string>;
using int_set    = neutron::flat_hash_set<int>;
using string_set = neutron::flat_hash_set<std::string>;

/// Hash with heavy collisions: four consecutive keys share a hash value.
struct grouped_hash {
    std::size_t operator()(int key) const noexcept {
        return static_cast<std::size_t>(key / 4);
    }
};

/// Every key collides.
struct constant_hash {
    std::size_t operator()(int) const noexcept { return 42; }
};

/// Standard hash that opts into prime bucket counts.
struct prime_hash : std::hash<int> {
    using hash_policy = neutron::prime_number_hash_policy;
};

/// Stateful hasher to check that it is stored and copied.
struct salted_hash {
    std::size_t salt = 0;
    std::size_t operator()(int key) const noexcept {
        return static_cast<std::size_t>(key) ^ salt;
    }
};

/// Stateful equality to check that it is stored and copied.
struct tagged_equal {
    int tag = 0;
    bool operator()(int lhs, int rhs) const noexcept { return lhs == rhs; }
};

/// Detects leaks, double destruction and use of destroyed objects.
struct tracked {
    static constexpr unsigned alive_magic = 0xA11CEu;
    static constexpr unsigned dead_magic  = 0xDEADu;

    static inline int live   = 0;
    static inline int errors = 0;

    int value      = 0;
    unsigned magic = alive_magic;

    tracked() noexcept { ++live; }
    tracked(int val) noexcept : value(val) { ++live; }
    tracked(const tracked& that) noexcept : value(that.value) {
        check(that);
        ++live;
    }
    tracked(tracked&& that) noexcept : value(that.value) {
        check(that);
        that.value = -1;
        ++live;
    }
    tracked& operator=(const tracked& that) noexcept {
        check(*this);
        check(that);
        value = that.value;
        return *this;
    }
    tracked& operator=(tracked&& that) noexcept {
        check(*this);
        check(that);
        value      = that.value;
        that.value = -1;
        return *this;
    }
    ~tracked() {
        check(*this);
        magic = dead_magic;
        --live;
    }

    static void check(const tracked& obj) noexcept {
        if (obj.magic != alive_magic) {
            ++errors;
        }
    }

    friend bool operator==(const tracked& lhs, const tracked& rhs) noexcept {
        check(lhs);
        check(rhs);
        return lhs.value == rhs.value;
    }
};

/// Hash for `tracked` that also validates the hashed object.
struct tracked_hash {
    std::size_t operator()(const tracked& obj) const noexcept {
        tracked::check(obj);
        return std::hash<int>{}(obj.value);
    }
};

/// Copy construction throws once the budget is exhausted.
struct throwing_copy {
    static inline int live   = 0;
    static inline int budget = -1;

    int value = 0;

    throwing_copy(int val) noexcept : value(val) { ++live; }
    throwing_copy(const throwing_copy& that) : value(that.value) {
        if (budget == 0) {
            throw std::runtime_error("copy");
        }
        if (budget > 0) {
            --budget;
        }
        ++live;
    }
    throwing_copy(throwing_copy&& that) noexcept : value(that.value) { ++live; }
    throwing_copy& operator=(const throwing_copy&) = default;
    throwing_copy& operator=(throwing_copy&&)      = default;
    ~throwing_copy() { --live; }

    friend bool operator==(
        const throwing_copy& lhs, const throwing_copy& rhs) noexcept {
        return lhs.value == rhs.value;
    }
};

/// Outstanding element count per allocator id.
inline long long outstanding[8];
/// Number of allocate() calls per allocator id.
inline long long allocations[8];

/// Allocator with an identity; tracks outstanding element counts per id.
template <typename Ty, bool Pocca, bool Pocma, bool Pocs>
struct id_allocator {
    using value_type                             = Ty;
    using propagate_on_container_copy_assignment = std::bool_constant<Pocca>;
    using propagate_on_container_move_assignment = std::bool_constant<Pocma>;
    using propagate_on_container_swap            = std::bool_constant<Pocs>;
    using is_always_equal                        = std::false_type;

    template <typename Other>
    struct rebind {
        using other = id_allocator<Other, Pocca, Pocma, Pocs>;
    };

    int id = 0;

    id_allocator(int ident) noexcept : id(ident) {}
    template <typename Other>
    id_allocator(const id_allocator<Other, Pocca, Pocma, Pocs>& that) noexcept
        : id(that.id) {}

    Ty* allocate(std::size_t count) {
        outstanding[id] += static_cast<long long>(count);
        ++allocations[id];
        return std::allocator<Ty>{}.allocate(count);
    }
    void deallocate(Ty* ptr, std::size_t count) noexcept {
        outstanding[id] -= static_cast<long long>(count);
        std::allocator<Ty>{}.deallocate(ptr, count);
    }

    template <typename Other>
    bool operator==(
        const id_allocator<Other, Pocca, Pocma, Pocs>& that) const noexcept {
        return id == that.id;
    }
};

/// True when every allocator id has released all its memory.
bool allocators_balanced() {
    return std::ranges::all_of(outstanding, [](long long val) {
        return val == 0;
    });
}

template <bool Pocca, bool Pocma, bool Pocs>
using id_alloc = id_allocator<std::pair<int, std::string>, Pocca, Pocma, Pocs>;

template <bool Pocca, bool Pocma, bool Pocs>
using id_map = neutron::flat_hash_map<
    int, std::string, std::hash<int>, std::equal_to<int>,
    id_alloc<Pocca, Pocma, Pocs>>;

using fixed_map   = id_map<false, false, false>;
using fixed_alloc = id_alloc<false, false, false>;

using propagating_map   = id_map<true, true, true>;
using propagating_alloc = id_alloc<true, true, true>;

/// Expected mapped value for `key`, as stored by `fill`.
template <typename Value>
Value value_for(int key) {
    if constexpr (std::same_as<Value, std::string>) {
        return std::to_string(key * 2);
    } else {
        return static_cast<Value>(key * 2);
    }
}

/// True when every key in [first, last) maps to `value_for(key)`.
template <typename Map>
bool contains_all(const Map& map, int first, int last) {
    using value_type = typename Map::mapped_type;
    for (int i = first; i != last; ++i) {
        auto iter = map.find(i);
        if (iter == map.end() || iter->first != i ||
            iter->second != value_for<value_type>(i)) {
            return false;
        }
    }
    return true;
}

/// Emplaces every key in [first, last) with `value_for(key)`.
template <typename Map>
void fill(Map& map, int first, int last) {
    for (int i = first; i != last; ++i) {
        map.emplace(i, value_for<typename Map::mapped_type>(i));
    }
}

/// Number of elements reached by iterating the range.
template <typename Range>
std::size_t distance_of(const Range& range) {
    return static_cast<std::size_t>(
        std::distance(std::ranges::begin(range), std::ranges::end(range)));
}

/// Trial-division primality check.
bool is_prime(std::size_t value) {
    if (value < 2) {
        return false;
    }
    for (std::size_t div = 2; div * div <= value; ++div) {
        if (value % div == 0) {
            return false;
        }
    }
    return true;
}

// Member types follow the std::unordered_map / std::unordered_set layout.
static_assert(std::same_as<int_map::key_type, int>);
static_assert(std::same_as<int_map::mapped_type, int>);
static_assert(std::same_as<int_map::value_type, std::pair<int, int>>);
static_assert(std::same_as<int_set::key_type, int>);
static_assert(std::same_as<int_set::value_type, int>);

// Iterators are forward iterators; iterator converts to const_iterator only.
static_assert(std::forward_iterator<int_map::iterator>);
static_assert(std::forward_iterator<int_map::const_iterator>);
static_assert(std::forward_iterator<string_set::iterator>);
static_assert(std::forward_iterator<string_set::const_iterator>);
static_assert(std::ranges::forward_range<int_map>);
static_assert(std::ranges::forward_range<const int_map>);
static_assert(std::ranges::forward_range<string_set>);
static_assert(std::same_as<
              std::iter_reference_t<int_map::const_iterator>,
              const std::pair<int, int>&>);
static_assert(std::same_as<
              std::iter_reference_t<int_map::iterator>, std::pair<int, int>&>);
static_assert(std::convertible_to<int_map::iterator, int_map::const_iterator>);
static_assert(!std::convertible_to<int_map::const_iterator, int_map::iterator>);

// Copy and move operations are available.
static_assert(std::is_nothrow_move_constructible_v<int_map>);
static_assert(std::is_copy_constructible_v<string_map>);
static_assert(std::is_copy_assignable_v<string_map>);
static_assert(std::is_move_assignable_v<string_map>);

// Moving with a possibly unequal allocator may allocate, so it may throw.
static_assert(!std::is_nothrow_constructible_v<
              fixed_map, fixed_map&&, const fixed_alloc&>);
static_assert(!std::is_nothrow_move_assignable_v<fixed_map>);
static_assert(std::is_nothrow_move_assignable_v<propagating_map>);
static_assert(std::is_nothrow_constructible_v<
              int_map, int_map&&, const std::allocator<std::pair<int, int>>&>);
static_assert(std::is_nothrow_move_assignable_v<int_map>);

void test_default_map_is_empty() {
    int_map map;
    const auto& cmap = map;

    require(map.empty());
    require(map.size() == 0);
    require(map.bucket_count() == 0);
    require(map.begin() == map.end());
    require(cmap.cbegin() == cmap.cend());
    require(map.load_factor() == 0.0F);
    require(map.max_load_factor() == 0.5F);
}

void test_lookup_in_empty_map_finds_nothing() {
    int_map map;
    const auto& cmap = map;

    const auto range = map.equal_range(1);

    require(map.find(1) == map.end());
    require(cmap.count(1) == 0);
    require(map.erase(1) == 0);
    require(range.first == map.end());
    require(range.second == map.end());
}

void test_maintenance_on_empty_map_is_harmless() {
    int_map map;

    map.clear();
    map.shrink_to_fit();
    map.rehash(0);

    require(map.empty());
    require(map.begin() == map.end());
}

void test_insert_rejects_duplicate_keys() {
    int_map map;

    auto [iter, inserted] = map.insert({ 1, 10 });
    auto dup              = map.insert({ 1, 99 });

    require(inserted);
    require(iter->first == 1);
    require(iter->second == 10);
    // The duplicate points at the existing element and keeps the old value.
    require_false(dup.second);
    require(dup.first == iter);
    require(dup.first->second == 10);
}

void test_emplace_rejects_duplicate_keys() {
    int_map map;

    require(map.emplace(2, 20).second);
    require_false(map.emplace(2, 21).second);
    require(map.at(2) == 20);
}

void test_insert_accepts_lvalue_and_rvalue_pairs() {
    int_map map;
    std::pair<int, int> value{ 3, 30 };

    require(map.insert(value).second);
    require_false(map.insert(std::move(value)).second);
    require(map.size() == 1);
}

void test_subscript_inserts_or_accesses() {
    int_map map;

    map[4]  = 40;
    int key = 5;
    map[std::move(key)] += 50;

    require(map[4] == 40);
    require(map[5] == 50);
    require(map.size() == 2);
    // A missing key is value-initialized.
    require(map[6] == 0);
    require(map.size() == 3);
    require(distance_of(map) == 3);
}

void test_at_reads_and_writes_existing_value() {
    int_map map{
        { 1, 10 },
        { 2, 20 }
    };

    require(map.at(1) == 10);
    require(std::as_const(map).at(2) == 20);

    map.at(1) = 11;

    auto found = map.find(1);
    require_or_return(found != map.end(), );
    require(found->second == 11);
}

void test_empty_emplace_inserts_default_value() {
    int_map map;

    auto [zero, zero_inserted] = map.emplace();

    require(zero_inserted);
    require(zero->first == 0);
    require(zero->second == 0);
    require(map.size() == 1);
}

void test_count_and_equal_range_match_presence() {
    int_map map;
    fill(map, 0, 7);

    const auto range  = map.equal_range(3);
    const auto crange = std::as_const(map).equal_range(100);

    require(map.count(3) == 1);
    require(map.count(100) == 0);
    require_or_return(range.first != map.end(), );
    require(range.first->first == 3);
    require(std::next(range.first) == range.second);
    require(crange.first == map.end());
    require(crange.second == map.end());
}

void test_hinted_insert_and_emplace() {
    int_map map;
    const std::pair<int, int> value{ 2, 2 };

    auto iter = map.insert(map.cend(), { 1, 1 });

    require(iter->first == 1);
    require(iter->second == 1);
    require(map.insert(map.cbegin(), value)->second == 2);
    require(map.emplace_hint(map.cend(), 3, 3)->second == 3);
    // A hinted duplicate returns the existing element.
    require(map.emplace_hint(map.cend(), 3, 4)->second == 3);
}

void test_insert_or_assign_overwrites_existing() {
    int_map map{
        { 1, 1 },
        { 2, 2 }
    };
    int key = 4;

    auto assigned = map.insert_or_assign(1, 100);
    require_false(assigned.second);
    require(assigned.first->second == 100);

    assigned = map.insert_or_assign(std::move(key), 400);
    require(assigned.second);
    require(assigned.first->second == 400);

    require(map.insert_or_assign(map.cend(), 2, 200)->second == 200);
    require(map.insert_or_assign(map.cend(), 5, 500)->second == 500);
    require(map.at(1) == 100);
}

void test_bulk_insert_keeps_first_duplicate() {
    int_map map;
    const std::vector<std::pair<int, int>> more{
        { 8, 8 },
        { 9, 9 }
    };

    map.insert(
        {
            { 6,  6 },
            { 7,  7 },
            { 6, 66 }
    });
    map.insert(more.begin(), more.end());

    require(map.size() == 4);
    require(map.at(6) == 6);
    require(map.at(9) == 9);
}

void test_at_throws_for_missing_key() {
    int_map map{
        { 1, 1 }
    };
    bool thrown       = false;
    bool const_thrown = false;

    ATOM_TRY { static_cast<void>(map.at(2)); }
    ATOM_CATCH(const std::out_of_range&) { thrown = true; }
    ATOM_TRY { static_cast<void>(std::as_const(map).at(3)); }
    ATOM_CATCH(const std::out_of_range&) { const_thrown = true; }

    require(thrown);
    require(const_thrown);
    require(map.size() == 1);
}

void test_growth_keeps_load_factor_bounded() {
    constexpr int count = 10000;
    int_map map;
    std::size_t last_buckets = 0;
    std::size_t growths      = 0;

    for (int i = 0; i != count; ++i) {
        map.emplace(i, i * 2);
        if (map.bucket_count() != last_buckets) {
            last_buckets = map.bucket_count();
            ++growths;
        }
        require(map.load_factor() <= map.max_load_factor());
    }

    require(growths >= 2);
    require(map.size() == count);
    require(contains_all(map, 0, count));
}

void test_default_policy_uses_power_of_two_buckets() {
    constexpr int count = 10000;
    int_map map;

    fill(map, 0, count);

    // Fibonacci hashing uses power-of-two bucket counts.
    require(std::has_single_bit(map.bucket_count()));
    require(map.bucket(123) < map.bucket_count());
}

void test_iteration_visits_every_element_once() {
    constexpr int count = 10000;
    int_map map;
    fill(map, 0, count);
    long long sum       = 0;
    std::size_t visited = 0;

    for (const auto& [key, value] : std::as_const(map)) {
        sum += key;
        ++visited;
        require(value == key * 2);
    }

    require(visited == count);
    require(sum == static_cast<long long>(count) * (count - 1) / 2);
}

void test_reserve_preallocates_buckets() {
    constexpr int count = 1000;
    int_map map;

    map.reserve(count);
    const auto reserved = map.bucket_count();
    fill(map, 0, count);

    // The default max load factor is 0.5.
    require(reserved >= 2 * count);
    require(std::has_single_bit(reserved));
    require(map.bucket_count() == reserved);
    require(contains_all(map, 0, count));
}

void test_rehash_never_drops_below_required_buckets() {
    constexpr int count = 1000;
    int_map map;
    fill(map, 0, count);

    map.rehash(10000);
    require(map.bucket_count() >= 10000);
    require(contains_all(map, 0, count));

    map.rehash(1);
    require(map.bucket_count() >= 2 * count);
    require(contains_all(map, 0, count));
}

void test_shrink_to_fit_after_erase_releases_buckets() {
    constexpr int count = 1000;
    constexpr int kept  = 10;
    int_map map;
    fill(map, 0, count);

    for (int i = kept; i != count; ++i) {
        require(map.erase(i) == 1);
    }
    map.shrink_to_fit();

    require(map.size() == kept);
    require(map.bucket_count() <= 32);
    require(contains_all(map, 0, kept));
}

void test_clear_keeps_buckets_until_shrink() {
    int_map map;
    fill(map, 0, 10);

    map.clear();
    const auto cleared = map.bucket_count();

    require(map.empty());
    require(cleared != 0);
    require(map.begin() == map.end());

    map.shrink_to_fit();
    require(map.bucket_count() == 0);

    fill(map, 0, 100);
    require(contains_all(map, 0, 100));
}

void test_larger_max_load_factor_needs_fewer_buckets() {
    constexpr int count = 900;
    int_map dense;
    int_map sparse;

    dense.max_load_factor(0.9F);
    require(dense.max_load_factor() == 0.9F);

    fill(dense, 0, count);
    fill(sparse, 0, count);

    require(dense.load_factor() <= 0.9F);
    require(dense.bucket_count() < sparse.bucket_count());
    require(contains_all(dense, 0, count));
}

void test_erase_by_key_returns_removed_count() {
    constexpr int count = 2000;
    int_map map;
    fill(map, 0, count);

    for (int i = 0; i < count; i += 3) {
        require(map.erase(i) == 1);
        require(map.erase(i) == 0);
    }

    for (int i = 0; i != count; ++i) {
        const bool expected = (i % 3) != 0;
        require((map.count(i) == 1) == expected);
    }
}

void test_erase_found_iterator_removes_element() {
    int_map map;
    fill(map, 0, 10);

    auto found = map.find(1);
    require_or_return(found != map.end(), );
    map.erase(found);

    require(map.count(1) == 0);
    require(map.size() == 9);
}

void test_erase_while_iterating_visits_each_once() {
    constexpr int count = 2000;
    int_map map;
    fill(map, 0, count);
    // Holes from erasure make the iteration skip empty slots.
    for (int i = 0; i < count; i += 3) {
        map.erase(i);
    }
    const auto before   = map.size();
    std::size_t visited = 0;
    std::size_t removed = 0;

    for (auto iter = map.begin(); iter != map.end();) {
        ++visited;
        if (iter->first % 2 == 0) {
            iter = map.erase(iter);
            ++removed;
        } else {
            ++iter;
        }
    }

    require(visited == before);
    require(map.size() == before - removed);
    for (const auto& [key, value] : map) {
        require(key % 2 != 0);
        require(key % 3 != 0);
        require(value == key * 2);
    }
}

void test_reinsert_after_erase_restores_all() {
    constexpr int count = 2000;
    int_map map;
    fill(map, 0, count);
    for (int i = 0; i < count; i += 2) {
        map.erase(i);
    }

    fill(map, 0, count);

    require(map.size() == count);
    require(contains_all(map, 0, count));
}

void test_map_is_usable_after_erasing_everything() {
    constexpr int count = 2000;
    int_map map;
    fill(map, 0, count);

    for (int i = 0; i != count; ++i) {
        map.erase(i);
    }

    require(map.empty());
    require(map.begin() == map.end());

    fill(map, 0, 10);
    require(contains_all(map, 0, 10));
    require(map.size() == 10);
}

/// Erases many [start, start + length) iterator ranges from fresh maps.
template <typename Map>
void check_range_erase(int count) {
    for (std::size_t first = 0; first < static_cast<std::size_t>(count);
         first += 7) {
        for (std::size_t length : { 0U, 1U, 2U, 5U, 13U, 40U }) {
            Map map;
            fill(map, 0, count);
            const auto size  = map.size();
            const auto start = std::min(first, size);
            const auto stop  = std::min(start + length, size);

            auto begin_it = std::next(map.begin(), static_cast<long>(start));
            auto end_it   = std::next(map.begin(), static_cast<long>(stop));
            std::vector<bool> erased(static_cast<std::size_t>(count));
            for (auto iter = begin_it; iter != end_it; ++iter) {
                erased[static_cast<std::size_t>(iter->first)] = true;
            }
            const bool to_end  = end_it == map.end();
            const int next_key = to_end ? -1 : end_it->first;

            auto result = map.erase(begin_it, end_it);

            require(map.size() == size - (stop - start));
            // The result points at the element that followed the range.
            if (to_end) {
                require(result == map.end());
            } else {
                require_or_return(result != map.end(), );
                require(result->first == next_key);
            }
            for (int i = 0; i != count; ++i) {
                const bool found =
                    map.find(i) != map.end() && map.find(i)->second == i * 2;
                require(found != erased[static_cast<std::size_t>(i)]);
            }
            require(distance_of(map) == map.size());
        }
    }
}

void test_range_erase_with_std_hash() { check_range_erase<int_map>(100); }

void test_range_erase_with_colliding_hash() {
    check_range_erase<neutron::flat_hash_map<int, int, grouped_hash>>(100);
}

void test_erase_full_range_empties_map() {
    int_map map;
    fill(map, 0, 50);

    auto result = map.erase(map.cbegin(), map.cend());

    require(result == map.end());
    require(map.empty());

    fill(map, 0, 50);
    require(contains_all(map, 0, 50));
}

/// Fills, erases odd keys and refills a map using the given hasher.
template <typename Map>
void check_erase_and_reinsert(int count) {
    Map map;

    fill(map, 0, count);
    require(map.size() == static_cast<std::size_t>(count));
    require(contains_all(map, 0, count));

    // Remove odd keys so that backward shifting crosses clusters.
    for (int i = 1; i < count; i += 2) {
        require(map.erase(i) == 1);
    }
    for (int i = 0; i != count; ++i) {
        require((map.count(i) == 1) == (i % 2 == 0));
    }

    fill(map, 0, count);
    require(contains_all(map, 0, count));
    require(distance_of(map) == static_cast<std::size_t>(count));
}

void test_grouped_collisions_survive_erase_and_reinsert() {
    check_erase_and_reinsert<neutron::flat_hash_map<int, int, grouped_hash>>(
        2000);
}

void test_full_collisions_survive_erase_and_reinsert() {
    check_erase_and_reinsert<neutron::flat_hash_map<int, int, constant_hash>>(
        6);
}

void test_erase_inside_single_cluster_keeps_others() {
    // Every key lands in the same bucket.
    neutron::flat_hash_map<int, int, constant_hash> map;
    fill(map, 0, 6);

    map.erase(2);
    map.erase(0);

    require(map.size() == 4);
    require(contains_all(map, 3, 6));
    require(map.count(1) == 1);
    require(map.count(0) == 0);
    require(map.count(2) == 0);
}

using pow2_map =
    neutron::flat_hash_map<int, int, neutron::power_of_two_std_hash<int>>;
using prime_map = neutron::flat_hash_map<int, int, prime_hash>;

void test_power_of_two_policy_survives_erase_and_reinsert() {
    check_erase_and_reinsert<pow2_map>(3000);
}

void test_prime_policy_survives_erase_and_reinsert() {
    check_erase_and_reinsert<prime_map>(3000);
}

void test_policies_choose_matching_bucket_counts() {
    constexpr int count = 1000;
    pow2_map pow2;
    prime_map prime;

    fill(pow2, 0, count);
    fill(prime, 0, count);

    require(std::has_single_bit(pow2.bucket_count()));
    require(is_prime(prime.bucket_count()));
    require(pow2.bucket(999) < pow2.bucket_count());
    require(prime.bucket(999) < prime.bucket_count());
}

void test_prime_policy_shrinks_to_zero_and_regrows() {
    prime_map prime;
    fill(prime, 0, 1000);

    prime.clear();
    prime.shrink_to_fit();

    require(prime.bucket_count() == 0);
    require(prime.find(1) == prime.end());

    fill(prime, 0, 10);
    require(contains_all(prime, 0, 10));
}

/// Map with 200 string keys and 40-character values.
string_map make_string_source() {
    constexpr int count = 200;
    string_map source;
    for (int i = 0; i != count; ++i) {
        source.emplace(std::to_string(i), std::string(40, 'a' + (i % 26)));
    }
    return source;
}

void test_copy_construction_is_deep() {
    string_map source = make_string_source();

    string_map copy(source);
    require(copy == source);
    require_false(copy != source);
    require(copy.size() == 200);

    copy["0"] = "changed";
    require_false(copy == source);
    require(source["0"] == std::string(40, 'a'));
}

void test_copy_assignment_replaces_contents() {
    const string_map source = make_string_source();
    string_map assigned{
        { "x", "y" }
    };

    assigned = source;
    require(assigned == source);
    require(assigned.count("x") == 0);

    // Alias avoids -Wself-assign while still assigning to itself.
    auto& self = assigned;
    assigned   = self;
    require(assigned == source);
}

void test_move_construction_leaves_source_empty_and_reusable() {
    const string_map source = make_string_source();
    string_map donor(source);

    string_map moved(std::move(donor));

    require(moved == source);
    require(donor.empty());
    require(donor.begin() == donor.end());

    donor.emplace("reuse", "ok");
    require(donor.size() == 1);
    require(donor.at("reuse") == "ok");
}

void test_move_assignment_replaces_contents() {
    const string_map source = make_string_source();
    string_map donor(source);
    string_map move_assigned{
        { "old", "value" }
    };

    move_assigned = std::move(donor);
    require(move_assigned == source);
    require(move_assigned.count("old") == 0);
    require(donor.empty());

    // Alias avoids -Wself-move; self move assignment must keep the contents.
    auto& self_move = move_assigned;
    move_assigned   = std::move(self_move);
    require(move_assigned == source);
}

void test_swap_exchanges_contents() {
    const string_map source = make_string_source();
    string_map mine(source);
    string_map other{
        { "a", "b" }
    };

    other.swap(mine);
    require(other == source);
    require(mine.size() == 1);
    require(mine.at("a") == "b");

    std::swap(other, mine);
    require(mine == source);
    require(other.at("a") == "b");
}

void test_copies_of_empty_map_are_empty() {
    const string_map empty;
    string_map empty_assigned{
        { "k", "v" }
    };

    string_map empty_copy(empty);
    empty_assigned = empty;

    require(empty_copy.empty());
    require(empty_assigned.empty());
    require(empty_copy == empty);
    require(empty_assigned.begin() == empty_assigned.end());
}

void test_equality_ignores_insertion_order_and_capacity() {
    constexpr int count = 100;
    int_map lhs;
    int_map rhs;
    rhs.reserve(1000);

    for (int i = 0; i != count; ++i) {
        lhs.emplace(i, i);
        rhs.emplace(count - 1 - i, count - 1 - i);
    }

    require(lhs == rhs);
    require_false(lhs != rhs);
    require(int_map{} == int_map{});
}

void test_equality_detects_value_and_size_differences() {
    int_map lhs;
    int_map rhs;
    for (int i = 0; i != 100; ++i) {
        lhs.emplace(i, i);
        rhs.emplace(i, i);
    }

    rhs[50] = -1;
    require_false(lhs == rhs);

    rhs.erase(50);
    require(lhs != rhs);
}

/// Pairs with keys 1..3 plus a later duplicate of key 1.
std::vector<std::pair<int, int>> pairs_with_duplicate() {
    return {
        { 1,   2 },
        { 2,   4 },
        { 3,   6 },
        { 1, 100 }
    };
}

void test_range_constructor_keeps_first_duplicate() {
    const auto values = pairs_with_duplicate();

    int_map from_range(values.begin(), values.end());

    require(from_range.size() == 3);
    require(from_range.at(1) == 2);
    require(contains_all(from_range, 1, 4));
}

void test_initializer_list_constructor_matches_range_constructor() {
    const auto values = pairs_with_duplicate();
    const int_map from_range(values.begin(), values.end());

    const int_map from_list{
        { 1, 2 },
        { 2, 4 },
        { 3, 6 }
    };

    require(from_list == from_range);
    require(from_list.bucket_count() != 0);
}

void test_bucket_count_constructors_preallocate() {
    const int_map with_buckets(100);
    const int_map list_buckets(
        {
            { 1, 2 }
    },
        64);

    require(with_buckets.bucket_count() >= 100);
    require(with_buckets.empty());
    require(list_buckets.bucket_count() >= 64);
    require(list_buckets.at(1) == 2);
}

/// Inserts six entries through every insertion form taking string-like keys.
void insert_string_forms(string_map& map) {
    map.emplace("alpha", "1");
    map.emplace(std::string("beta"), "2");
    map.emplace(std::make_pair("gamma", "3"));
    map.insert(std::make_pair(std::string("delta"), std::string("4")));
    map["epsilon"]      = "5";
    std::string key     = "zeta";
    map[std::move(key)] = "6";
}

void test_string_keys_accept_every_insertion_form() {
    string_map map;

    insert_string_forms(map);

    require(map.size() == 6);
    require(map.at("alpha") == "1");
    require(map.at("gamma") == "3");
    require(map.at("zeta") == "6");
    require_false(map.emplace("alpha", "x").second);
    require_false(map.emplace(std::make_pair("gamma", "y")).second);
}

void test_long_string_keys_survive_rehash() {
    // Long strings exercise moves of heap buffers during rehash.
    constexpr int count = 500;
    const auto long_key = [](int idx) {
        return std::string(64, 'k') + std::to_string(idx);
    };
    string_map map;
    insert_string_forms(map);

    for (int i = 0; i != count; ++i) {
        map.emplace(long_key(i), std::to_string(i));
    }

    for (int i = 0; i != count; ++i) {
        auto iter = map.find(long_key(i));
        require_or_return(iter != map.end(), );
        require(iter->second == std::to_string(i));
    }
    require(map.erase("alpha") == 1);
    require(map.count("alpha") == 0);
    require(map.size() == 505);
}

void test_map_operations_balance_object_lifetimes() {
    {
        neutron::flat_hash_map<tracked, tracked, tracked_hash> map;
        for (int i = 0; i != 1000; ++i) {
            map.emplace(tracked{ i }, tracked{ i * 2 });
        }
        for (int i = 0; i < 1000; i += 2) {
            map.erase(tracked{ i });
        }
        map[tracked{ 5000 }].value = 7;

        auto copy = map;
        copy.rehash(4096);
        copy.shrink_to_fit();
        auto moved = std::move(copy);
        copy       = moved;
        moved.clear();
        moved = std::move(copy);
        map.erase(map.begin(), std::next(map.begin(), 10));
        for (auto iter = map.begin(); iter != map.end();) {
            iter = map.erase(iter);
        }

        require(map.empty());
        require(moved.size() == 501);
        require(moved.at(tracked{ 5000 }).value == 7);
        require(moved.at(tracked{ 1 }).value == 2);
    }

    require(tracked::live == 0);
    require(tracked::errors == 0);
}

void test_set_operations_balance_object_lifetimes() {
    {
        neutron::flat_hash_set<tracked, tracked_hash> set;
        for (int i = 0; i != 300; ++i) {
            set.emplace(i);
            set.insert(tracked{ i });
        }

        auto set_copy = set;
        for (int i = 0; i < 300; i += 3) {
            set_copy.erase(tracked{ i });
        }

        require(set.size() == 300);
        require(set_copy.size() == 200);
    }

    require(tracked::live == 0);
    require(tracked::errors == 0);
}

void test_move_only_values_survive_moves() {
    constexpr int count = 300;
    neutron::flat_hash_map<int, std::unique_ptr<int>> map;
    for (int i = 0; i != count; ++i) {
        map.emplace(i, std::make_unique<int>(i));
    }
    map[1000] = std::make_unique<int>(1000);
    neutron::flat_hash_map<int, std::unique_ptr<int>> target;

    auto moved = std::move(map);
    target     = std::move(moved);

    require(target.size() == count + 1);
    require(*target.at(1000) == 1000);
    for (int i = 0; i != count; ++i) {
        auto iter = target.find(i);
        require_or_return(iter != target.end(), );
        require_or_return(iter->second != nullptr, );
        require(*iter->second == i);
    }
}

void test_set_stores_move_only_keys() {
    neutron::flat_hash_set<std::unique_ptr<int>> set;
    auto ptr  = std::make_unique<int>(1);
    auto* raw = ptr.get();

    set.insert(std::move(ptr));

    require_or_return(set.size() == 1, );
    require(set.begin()->get() == raw);
}

using throwing_map = neutron::flat_hash_map<int, throwing_copy>;

/// Map with keys 0..99 whose values are `throwing_copy{ key }`.
throwing_map make_throwing_source() {
    throwing_map source;
    for (int i = 0; i != 100; ++i) {
        source.emplace(i, throwing_copy{ i });
    }
    return source;
}

void test_failed_copy_construction_releases_everything() {
    {
        const throwing_map source = make_throwing_source();
        const int baseline        = throwing_copy::live;
        bool thrown               = false;

        throwing_copy::budget = 50;
        ATOM_TRY {
            throwing_map copy(source);
            static_cast<void>(copy);
        }
        ATOM_CATCH(const std::runtime_error&) { thrown = true; }
        throwing_copy::budget = -1;

        require(thrown);
        require(throwing_copy::live == baseline);
    }

    require(throwing_copy::live == 0);
}

void test_failed_insert_keeps_existing_contents() {
    {
        throwing_map source = make_throwing_source();
        const throwing_copy value{ 1000 };
        const std::pair<int, throwing_copy> entry{ 1000, value };
        bool thrown = false;

        throwing_copy::budget = 0;
        ATOM_TRY { source.insert(entry); }
        ATOM_CATCH(const std::runtime_error&) { thrown = true; }
        throwing_copy::budget = -1;

        require(thrown);
        require(source.size() == 100);
        require(source.count(1000) == 0);
        for (int i = 0; i != 100; ++i) {
            require(source.at(i).value == i);
        }
    }

    require(throwing_copy::live == 0);
}

void test_failed_copy_assignment_leaves_valid_map() {
    {
        const throwing_map source = make_throwing_source();
        throwing_map target{
            { 1, throwing_copy{ 1 } }
        };
        bool thrown = false;

        throwing_copy::budget = 10;
        ATOM_TRY { target = source; }
        ATOM_CATCH(const std::runtime_error&) { thrown = true; }
        throwing_copy::budget = -1;

        require(thrown);
        require(target.size() <= source.size());
        require(distance_of(target) == target.size());

        target = source;
        require(target == source);
    }

    require(throwing_copy::live == 0);
}

/// Map using allocator id 1, filled with keys 0..99.
fixed_map make_fixed_source() {
    fixed_map first(fixed_alloc{ 1 });
    fill(first, 0, 100);
    return first;
}

void test_map_allocates_through_its_allocator() {
    {
        const fixed_map first = make_fixed_source();

        const fixed_alloc rebound = first.get_allocator();

        require(first.get_allocator().id == 1);
        require(outstanding[1] != 0);
        require(outstanding[2] == 0);
        require(contains_all(first, 0, 100));
        require(rebound.id == 1);
    }

    require(allocators_balanced());
}

void test_copy_construction_selects_allocator() {
    {
        const fixed_map first = make_fixed_source();

        const fixed_map copy(first);
        const fixed_map copy_other(first, fixed_alloc{ 2 });

        require(copy.get_allocator().id == 1);
        require(copy_other.get_allocator().id == 2);
        require(outstanding[2] != 0);
        require(copy_other == first);
        require(copy == first);
    }

    require(allocators_balanced());
}

void test_move_construction_keeps_allocator() {
    {
        const fixed_map first = make_fixed_source();
        fixed_map copy(first);

        const fixed_map moved(std::move(copy));

        require(moved.get_allocator().id == 1);
        require(moved == first);
    }

    require(allocators_balanced());
}

void test_allocator_extended_move_respects_allocator_equality() {
    {
        const fixed_map first = make_fixed_source();
        fixed_map source(first, fixed_alloc{ 6 });

        // An unequal allocator moves the elements, so each buffer is
        // released by the allocator owning it.
        fixed_map moved_other(std::move(source), fixed_alloc{ 7 });
        require(moved_other.get_allocator().id == 7);
        require(moved_other == first);
        require(source.empty());

        // An equal allocator steals the buffer.
        const auto before_steal = allocations[7];
        const fixed_map stolen(std::move(moved_other), fixed_alloc{ 7 });
        require(stolen == first);
        require(allocations[7] == before_steal);
    }

    require(allocators_balanced());
}

void test_non_propagating_move_assignment_respects_allocator_equality() {
    {
        const fixed_map first = make_fixed_source();
        fixed_map moved(first);
        fixed_map target(fixed_alloc{ 3 });
        target.emplace(-1, "x");

        // Unequal allocators without propagation move element-wise.
        target = std::move(moved);
        require(target.get_allocator().id == 3);
        require(target == first);
        require(moved.empty());
        require(moved.get_allocator().id == 1);

        // Equal allocators without propagation steal the buffer.
        fixed_map same(fixed_alloc{ 3 });
        const auto before = allocations[3];
        same              = std::move(target);
        require(same == first);
        require(allocations[3] == before);
    }

    require(allocators_balanced());
}

void test_non_propagating_copy_assignment_keeps_own_allocator() {
    {
        const fixed_map first = make_fixed_source();
        fixed_map assigned(fixed_alloc{ 2 });

        assigned = first;

        require(assigned.get_allocator().id == 2);
        require(assigned == first);
    }

    require(allocators_balanced());
}

void test_propagating_copy_assignment_adopts_source_allocator() {
    {
        propagating_map first(propagating_alloc{ 4 });
        fill(first, 0, 50);
        propagating_map copy_assigned(propagating_alloc{ 6 });
        fill(copy_assigned, 0, 10);

        copy_assigned = first;

        require(copy_assigned.get_allocator().id == 4);
        require(copy_assigned == first);
        // The old buffer was released through the old allocator.
        require(outstanding[6] == 0);
    }

    require(allocators_balanced());
}

void test_propagating_move_assignment_adopts_source_allocator() {
    {
        propagating_map first(propagating_alloc{ 4 });
        fill(first, 0, 50);
        propagating_map donor(first);
        propagating_map move_assigned(propagating_alloc{ 6 });
        fill(move_assigned, 0, 10);

        move_assigned = std::move(donor);

        require(move_assigned.get_allocator().id == 4);
        require(move_assigned == first);
        require(outstanding[6] == 0);
    }

    require(allocators_balanced());
}

void test_propagating_swap_exchanges_allocators() {
    {
        propagating_map first(propagating_alloc{ 4 });
        fill(first, 0, 50);
        propagating_map second(propagating_alloc{ 5 });
        fill(second, 100, 120);

        first.swap(second);

        require(first.get_allocator().id == 5);
        require(second.get_allocator().id == 4);
        require(contains_all(first, 100, 120));
        require(contains_all(second, 0, 50));
    }

    require(allocators_balanced());
}

using salted_map = neutron::flat_hash_map<int, int, salted_hash, tagged_equal>;

constexpr std::size_t test_salt = 0x5A5A;
constexpr int test_tag          = 7;

void test_stateful_functors_are_stored() {
    salted_map source(16, salted_hash{ test_salt }, tagged_equal{ test_tag });

    fill(source, 0, 500);

    require(source.hash_function().salt == test_salt);
    require(source.key_eq().tag == test_tag);
    require(contains_all(source, 0, 500));
}

void test_stateful_functors_follow_copy_and_move() {
    salted_map source(16, salted_hash{ test_salt }, tagged_equal{ test_tag });
    fill(source, 0, 500);

    salted_map copy(source);
    salted_map moved(std::move(copy));
    salted_map assigned;
    assigned = moved;

    require(assigned.hash_function().salt == test_salt);
    require(assigned.key_eq().tag == test_tag);
    require(contains_all(assigned, 0, 500));
    require(moved.hash_function().salt == test_salt);
}

/// Set holding "a", "b", "c", "d" and "xxx".
string_set make_letter_set() {
    string_set set;
    set.insert("a");
    set.emplace(3, 'x');
    set.emplace("b");
    set.emplace("c");
    set.emplace("d");
    return set;
}

void test_set_insert_and_emplace_reject_duplicates() {
    string_set set;
    require(set.empty());
    require(set.begin() == set.end());

    require(set.insert("a").second);
    require_false(set.insert("a").second);

    auto [iter, inserted] = set.emplace(3, 'x');
    require(inserted);
    require(*iter == "xxx");

    const std::string bee = "b";
    std::string cee       = "c";
    require(set.emplace(bee).second);
    require(set.emplace(cee).second);
    require(set.emplace(std::string("d")).second);
    require_false(set.emplace("d").second);

    require(set.size() == 5);
    require(set.count("xxx") == 1);
    require(set.find("zzz") == set.end());
}

void test_set_erase_by_key_removes_only_that_key() {
    constexpr int count = 1000;
    string_set set      = make_letter_set();

    for (int i = 0; i != count; ++i) {
        set.insert(std::to_string(i));
    }
    require(set.size() == 1005);
    require(distance_of(set) == 1005);

    for (int i = 0; i < count; i += 2) {
        require(set.erase(std::to_string(i)) == 1);
    }
    for (int i = 0; i != count; ++i) {
        require((set.count(std::to_string(i)) == 1) == (i % 2 == 1));
    }
}

void test_set_copy_and_move() {
    const string_set set = make_letter_set();

    string_set copy(set);
    require(copy == set);
    require_false(copy != set);

    copy.erase("a");
    require_false(copy == set);

    string_set moved(std::move(copy));
    require(moved.size() == set.size() - 1);
    require(copy.empty());
}

void test_int_set_deduplicates_initializer_list() {
    int_set numbers{ 1, 2, 3, 2, 1 };
    const int_set expected{ 3, 2, 1 };
    int sum = 0;

    for (int val : std::as_const(numbers)) {
        sum += val;
    }

    require(numbers.size() == 3);
    require(numbers == expected);
    require(sum == 6);

    auto found = numbers.find(2);
    require_or_return(found != numbers.end(), );
    numbers.erase(found);
    require(numbers.size() == 2);
    require(numbers.count(2) == 0);
}

} // namespace

int main() {
    test_default_map_is_empty();
    test_lookup_in_empty_map_finds_nothing();
    test_maintenance_on_empty_map_is_harmless();

    test_insert_rejects_duplicate_keys();
    test_emplace_rejects_duplicate_keys();
    test_insert_accepts_lvalue_and_rvalue_pairs();
    test_subscript_inserts_or_accesses();
    test_at_reads_and_writes_existing_value();
    test_empty_emplace_inserts_default_value();
    test_count_and_equal_range_match_presence();
    test_hinted_insert_and_emplace();
    test_insert_or_assign_overwrites_existing();
    test_bulk_insert_keeps_first_duplicate();
    test_at_throws_for_missing_key();

    test_growth_keeps_load_factor_bounded();
    test_default_policy_uses_power_of_two_buckets();
    test_iteration_visits_every_element_once();
    test_reserve_preallocates_buckets();
    test_rehash_never_drops_below_required_buckets();
    test_shrink_to_fit_after_erase_releases_buckets();
    test_clear_keeps_buckets_until_shrink();
    test_larger_max_load_factor_needs_fewer_buckets();

    test_erase_by_key_returns_removed_count();
    test_erase_found_iterator_removes_element();
    test_erase_while_iterating_visits_each_once();
    test_reinsert_after_erase_restores_all();
    test_map_is_usable_after_erasing_everything();
    test_range_erase_with_std_hash();
    test_range_erase_with_colliding_hash();
    test_erase_full_range_empties_map();

    test_grouped_collisions_survive_erase_and_reinsert();
    test_full_collisions_survive_erase_and_reinsert();
    test_erase_inside_single_cluster_keeps_others();
    test_power_of_two_policy_survives_erase_and_reinsert();
    test_prime_policy_survives_erase_and_reinsert();
    test_policies_choose_matching_bucket_counts();
    test_prime_policy_shrinks_to_zero_and_regrows();

    test_copy_construction_is_deep();
    test_copy_assignment_replaces_contents();
    test_move_construction_leaves_source_empty_and_reusable();
    test_move_assignment_replaces_contents();
    test_swap_exchanges_contents();
    test_copies_of_empty_map_are_empty();
    test_equality_ignores_insertion_order_and_capacity();
    test_equality_detects_value_and_size_differences();

    test_range_constructor_keeps_first_duplicate();
    test_initializer_list_constructor_matches_range_constructor();
    test_bucket_count_constructors_preallocate();
    test_string_keys_accept_every_insertion_form();
    test_long_string_keys_survive_rehash();

    test_map_operations_balance_object_lifetimes();
    test_set_operations_balance_object_lifetimes();
    test_move_only_values_survive_moves();
    test_set_stores_move_only_keys();
    test_failed_copy_construction_releases_everything();
    test_failed_insert_keeps_existing_contents();
    test_failed_copy_assignment_leaves_valid_map();

    test_map_allocates_through_its_allocator();
    test_copy_construction_selects_allocator();
    test_move_construction_keeps_allocator();
    test_allocator_extended_move_respects_allocator_equality();
    test_non_propagating_move_assignment_respects_allocator_equality();
    test_non_propagating_copy_assignment_keeps_own_allocator();
    test_propagating_copy_assignment_adopts_source_allocator();
    test_propagating_move_assignment_adopts_source_allocator();
    test_propagating_swap_exchanges_allocators();

    test_stateful_functors_are_stored();
    test_stateful_functors_follow_copy_and_move();

    test_set_insert_and_emplace_reject_duplicates();
    test_set_erase_by_key_removes_only_that_key();
    test_set_copy_and_move();
    test_int_set_deduplicates_initializer_list();
    return neutron_test::result();
}
