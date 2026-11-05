#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <neutron/bytell_hash_map.hpp>
#include <neutron/utility.hpp>
#include "require.hpp"

namespace {

using neutron::bytell_hash_map;
using neutron::bytell_hash_set;

using int_map    = bytell_hash_map<int, int>;
using string_map = bytell_hash_map<std::string, std::string>;
using int_set    = bytell_hash_set<int>;

/// Tracks live instances to detect leaks and double destruction.
struct counted {
    inline static int live          = 0;
    inline static int bad_destroy   = 0;
    inline static int throw_on_copy = -1; ///< Throw when it reaches zero.

    int value = 0;
    bool alive{};

    counted() : alive(true) { ++live; }
    counted(int value) : value(value), alive(true) { ++live; }
    counted(const counted& that) : value(that.value) {
        if (throw_on_copy == 0) {
            throw std::runtime_error("copy");
        }
        if (throw_on_copy > 0) {
            --throw_on_copy;
        }
        alive = true;
        ++live;
    }
    counted(counted&& that) noexcept : value(that.value), alive(true) {
        ++live;
    }
    counted& operator=(const counted&) = default;
    counted& operator=(counted&&)      = default;
    ~counted() {
        if (!alive) {
            ++bad_destroy;
        }
        alive = false;
        --live;
    }

    bool operator==(const counted& that) const { return value == that.value; }
};

/// Hashes `counted` by its value.
struct counted_hash {
    std::size_t operator()(const counted& c) const {
        return std::hash<int>{}(c.value);
    }
};

using counted_set = bytell_hash_set<counted, counted_hash>;

/// Resets the `counted` bookkeeping before a test.
void reset_counted() {
    counted::live          = 0;
    counted::bad_destroy   = 0;
    counted::throw_on_copy = -1;
}

/// True when every `counted` was destroyed exactly once.
bool counted_clean() { return counted::live == 0 && counted::bad_destroy == 0; }

/// Bookkeeping shared by `tracking_allocator` copies.
struct arena {
    std::map<void*, std::size_t> blocks;
    int bad_deallocations = 0;
};

/// Allocator recording every live block in an `arena`.
template <typename T, bool Propagate = false>
struct tracking_allocator {
    using value_type = T;
    using propagate_on_container_copy_assignment =
        std::bool_constant<Propagate>;
    using propagate_on_container_move_assignment =
        std::bool_constant<Propagate>;
    using propagate_on_container_swap = std::bool_constant<Propagate>;

    template <typename U>
    struct rebind {
        using other = tracking_allocator<U, Propagate>;
    };

    arena* owner;

    explicit tracking_allocator(arena* owner) noexcept : owner(owner) {}
    template <typename U>
    tracking_allocator(const tracking_allocator<U, Propagate>& that) noexcept
        : owner(that.owner) {}

    T* allocate(std::size_t count) {
        T* ptr             = std::allocator<T>{}.allocate(count);
        owner->blocks[ptr] = count;
        return ptr;
    }

    void deallocate(T* ptr, std::size_t count) {
        auto iter = owner->blocks.find(ptr);
        if (iter == owner->blocks.end() || iter->second != count) {
            ++owner->bad_deallocations;
        } else {
            owner->blocks.erase(iter);
        }
        std::allocator<T>{}.deallocate(ptr, count);
    }

    template <typename U>
    bool operator==(const tracking_allocator<U, Propagate>& that) const {
        return owner == that.owner;
    }
};

using tracked_alloc = tracking_allocator<std::pair<int, std::string>>;
using tracked_map   = bytell_hash_map<
      int, std::string, std::hash<int>, std::equal_to<int>, tracked_alloc>;

using propagating_alloc = tracking_allocator<int, true>;
using propagating_set =
    bytell_hash_set<int, std::hash<int>, std::equal_to<int>, propagating_alloc>;

/// True when every block was returned with its original size.
bool arena_clean(const arena& a) {
    return a.blocks.empty() && a.bad_deallocations == 0;
}

/// Hash requesting the prime number bucket policy.
struct prime_hash : std::hash<int> {
    using hash_policy = neutron::prime_number_hash_policy;
};

/// Sends every key to the same bucket.
struct constant_hash {
    std::size_t operator()(int) const noexcept { return 42; }
};

/// Sends keys to only three buckets.
struct coarse_hash {
    std::size_t operator()(int key) const noexcept {
        return static_cast<std::size_t>(key % 3);
    }
};

/// Element type with a large alignment requirement.
struct alignas(64) over_aligned {
    int value;
    bool operator==(const over_aligned&) const = default;
};

/// Hashes `over_aligned` by its value.
struct over_aligned_hash {
    std::size_t operator()(const over_aligned& item) const noexcept {
        return static_cast<std::size_t>(item.value);
    }
};

/// True when the map holds exactly `key -> key * 2` for keys in
/// `[first, last)` with the given step.
template <typename Map>
bool contains_exactly(const Map& map, int first, int last, int step = 1) {
    std::size_t expected = 0;
    for (int key = first; key < last; key += step) {
        auto iter = map.find(key);
        if (iter == map.end() || iter->second != key * 2) {
            return false;
        }
        ++expected;
    }
    return map.size() == expected;
}

/// Inserts `key -> key * 2` for keys in `[first, last)` with the given step.
template <typename Map>
void fill_doubled(Map& map, int first, int last, int step = 1) {
    for (int key = first; key < last; key += step) {
        map.emplace(key, key * 2);
    }
}

/// Long strings force heap allocations so ASan catches leaks.
std::string long_string(int index) {
    return std::string(40, 'x') + std::to_string(index);
}

constexpr int long_string_count = 1000;

/// Maps `long_string(i)` to `long_string(i * 2)`, then erases even `i`.
string_map make_long_string_map() {
    string_map map;
    for (int i = 0; i < long_string_count; ++i) {
        map[long_string(i)] = long_string(i * 2);
    }
    for (int i = 0; i < long_string_count; i += 2) {
        map.erase(long_string(i));
    }
    return map;
}

constexpr int string_source_size = 100;

/// Maps `"0"`..`"99"` to a 30 character string.
string_map make_string_source() {
    string_map map;
    for (int i = 0; i < string_source_size; ++i) {
        map.emplace(std::to_string(i), std::string(30, 'v'));
    }
    return map;
}

/// Builds two equal maps with different insertion order and bucket count.
std::pair<int_map, int_map> make_equal_maps() {
    int_map lhs;
    int_map rhs;
    fill_doubled(lhs, 0, 50);
    rhs.reserve(1000);
    for (int key = 49; key >= 0; --key) {
        rhs.emplace(key, key * 2);
    }
    return { std::move(lhs), std::move(rhs) };
}

constexpr int tracked_size = 200;

/// Fills a tracked map allocating from `owner`.
tracked_map make_tracked_map(arena& owner) {
    tracked_map map{ tracked_alloc{ &owner } };
    for (int i = 0; i < tracked_size; ++i) {
        map.emplace(i, std::string(30, 'a'));
    }
    return map;
}

// Member types follow the standard unordered containers.
static_assert(std::is_same_v<int_map::key_type, int>);
static_assert(std::is_same_v<int_map::mapped_type, int>);
static_assert(std::is_same_v<int_map::value_type, std::pair<int, int>>);
static_assert(std::is_same_v<int_map::size_type, std::size_t>);
static_assert(std::is_same_v<int_set::key_type, int>);
static_assert(std::is_same_v<int_set::value_type, int>);

// Iterators are forward iterators and the containers are forward ranges.
static_assert(std::forward_iterator<int_map::iterator>);
static_assert(std::forward_iterator<int_map::const_iterator>);
static_assert(std::forward_iterator<int_set::iterator>);
static_assert(std::forward_iterator<int_set::const_iterator>);
static_assert(std::ranges::forward_range<string_map>);
static_assert(std::ranges::forward_range<const string_map>);
static_assert(std::ranges::common_range<string_map>);
static_assert(std::ranges::forward_range<int_set>);

// const_iterator yields const references and only iterator converts to it.
static_assert(std::is_same_v<
              std::iter_reference_t<int_map::const_iterator>,
              const std::pair<int, int>&>);
static_assert(
    std::is_convertible_v<int_map::iterator, int_map::const_iterator>);
static_assert(
    !std::is_convertible_v<int_map::const_iterator, int_map::iterator>);

// Copyable, and nothrow movable with the default allocator.
static_assert(std::is_copy_constructible_v<string_map>);
static_assert(std::is_nothrow_move_constructible_v<string_map>);
static_assert(std::is_copy_assignable_v<string_map>);
static_assert(std::is_move_assignable_v<string_map>);

// Moving with a possibly unequal allocator may allocate.
static_assert(!std::is_nothrow_constructible_v<
              tracked_map, tracked_map&&, const tracked_alloc&>);
static_assert(!std::is_nothrow_move_assignable_v<tracked_map>);
static_assert(std::is_nothrow_move_assignable_v<bytell_hash_map<
                  int, std::string, std::hash<int>, std::equal_to<int>,
                  tracking_allocator<std::pair<int, std::string>, true>>>);
static_assert(std::is_nothrow_constructible_v<
              bytell_hash_map<int, int>, bytell_hash_map<int, int>&&,
              const std::allocator<std::pair<int, int>>&>);

void test_default_constructed_map_is_empty() {
    int_map map;
    const int_map& cmap = map;

    require(map.empty());
    require(map.size() == 0);
    require(map.bucket_count() == 0);
    require(map.load_factor() == 0.0F);
    require(map.max_size() != 0);
    require(map.begin() == map.end());
    require(cmap.begin() == cmap.end());
    require(map.cbegin() == map.cend());
}

void test_lookup_in_empty_map_finds_nothing() {
    int_map map;
    const int_map& cmap = map;

    require(map.find(1) == map.end());
    require(cmap.find(1) == cmap.end());
    require(map.count(1) == 0);
    require(map.erase(1) == 0);
}

void test_clear_and_shrink_on_empty_map_keep_it_empty() {
    int_map map;

    map.clear();
    map.shrink_to_fit();

    require(map.empty());
    require(map.begin() == map.end());
}

void test_insert_returns_inserted_element() {
    int_map map;

    auto [iter, inserted] = map.insert({ 1, 10 });

    require(inserted);
    require(iter->first == 1);
    require(iter->second == 10);
    require(map.size() == 1);
    require_false(map.empty());
}

void test_emplace_keeps_existing_value_for_duplicate_key() {
    int_map map;
    auto original = map.insert({ 1, 10 }).first;

    auto [iter, inserted] = map.emplace(1, 99);

    require_false(inserted);
    require(iter == original);
    require(iter->second == 10);
}

void test_insert_lvalue_rejects_duplicate_key() {
    int_map map;
    const std::pair<int, int> value{ 2, 20 };

    const bool first_inserted  = map.insert(value).second;
    const bool second_inserted = map.insert(value).second;

    require(first_inserted);
    require_false(second_inserted);
    require(map.at(2) == 20);
}

void test_hinted_insert_returns_element() {
    int_map map{
        { 1, 10 },
        { 2, 20 }
    };

    auto emplaced = map.emplace_hint(map.begin(), 3, 30);
    auto inserted = map.insert(map.cbegin(), std::pair{ 4, 40 });

    require(emplaced->second == 30);
    require(inserted->second == 40);
    require(map.at(3) == 30);
    require(map.at(4) == 40);
}

void test_subscript_inserts_default_value() {
    int_map map;

    map[5] = 50;
    ++map[5];

    require(map[5] == 51);
    require(map[6] == 0);
    require(map.size() == 2);
}

void test_emplace_without_arguments_inserts_default_pair() {
    int_map map;

    const bool inserted = map.emplace().second;

    require(inserted);
    require(map.at(0) == 0);
}

void test_equal_range_finds_single_element() {
    int_map map{
        { 1, 10 },
        { 2, 20 },
        { 3, 30 }
    };

    auto range = map.equal_range(2);

    require_or_return(std::distance(range.first, range.second) == 1, );
    require(range.first->second == 20);
}

void test_equal_range_of_missing_key_is_end() {
    int_map map{
        { 1, 10 },
        { 2, 20 }
    };

    auto missing = map.equal_range(100);

    require(missing.first == map.end());
    require(missing.second == map.end());
}

void test_const_equal_range_finds_single_element() {
    const int_map map{
        { 1, 10 },
        { 3, 30 }
    };

    auto range = map.equal_range(3);

    require_or_return(range.first != map.end(), );
    require(range.first->second == 30);
    require(std::next(range.first) == range.second);
}

void test_count_at_and_bucket_on_populated_map() {
    int_map map{
        { 1, 10 },
        { 4, 40 }
    };
    const int_map& cmap = map;

    require(map.count(4) == 1);
    require(map.count(100) == 0);
    require(cmap.at(4) == 40);
    require(map.bucket(4) < map.bucket_count());
}

void test_at_throws_out_of_range_for_missing_key() {
    int_map map{
        { 1, 1 }
    };
    const int_map& cmap = map;
    bool thrown         = false;
    bool const_thrown   = false;

    ATOM_TRY { static_cast<void>(map.at(2)); }
    ATOM_CATCH(const std::out_of_range&) { thrown = true; }
    ATOM_TRY { static_cast<void>(cmap.at(2)); }
    ATOM_CATCH(const std::out_of_range&) { const_thrown = true; }

    require(thrown);
    require(const_thrown);
    require(map.size() == 1);
}

void test_insert_or_assign_inserts_then_assigns() {
    string_map map;
    const std::string key = "key";

    auto first = map.insert_or_assign(key, "a");
    require(first.second);
    require(first.first->second == "a");
    auto second = map.insert_or_assign(std::string("key"), std::string("b"));

    require_false(second.second);
    require(second.first->second == "b");
    require(map.size() == 1);
}

void test_hinted_insert_or_assign_inserts_and_assigns() {
    string_map map{
        { "key", "b" }
    };
    const std::string key = "key";

    auto assigned = map.insert_or_assign(map.cbegin(), key, "c");
    require(assigned->second == "c");
    auto inserted =
        map.insert_or_assign(map.cbegin(), std::string("other"), "d");

    require(inserted->first == "other");
    require(inserted->second == "d");
    require(map.size() == 2);
}

const std::vector<std::pair<int, int>> pairs_with_duplicate{
    { 1, 2 },
    { 2, 4 },
    { 3, 6 },
    { 1, 8 }
};

void test_range_constructor_keeps_first_duplicate() {
    const int_map map(pairs_with_duplicate.begin(), pairs_with_duplicate.end());

    require(contains_exactly(map, 1, 4));
}

void test_initializer_list_constructor_matches_range_constructor() {
    const int_map from_range(
        pairs_with_duplicate.begin(), pairs_with_duplicate.end());

    const int_map from_list{
        { 1, 2 },
        { 2, 4 },
        { 3, 6 }
    };

    require(contains_exactly(from_list, 1, 4));
    require(from_list == from_range);
}

void test_bucket_count_constructor_reserves_buckets() {
    constexpr std::size_t buckets = 100;

    const int_map map(buckets);

    require(map.bucket_count() >= buckets);
    require(map.empty());
}

void test_initializer_list_with_bucket_count_constructor() {
    constexpr std::size_t buckets = 64;

    const int_map map(
        {
            { 1, 2 }
    },
        buckets);

    require(map.bucket_count() >= buckets);
    require(contains_exactly(map, 1, 2));
}

void test_set_initializer_list_drops_duplicates() {
    const int_set set{ 1, 2, 3, 2 };

    require(set.size() == 3);
    require(set.count(2) == 1);
}

constexpr int growth_count = 10000;

void test_insert_grows_within_max_load_factor() {
    int_map map;
    std::size_t last_buckets = map.bucket_count();
    int rehashes             = 0;

    for (int key = 0; key < growth_count; ++key) {
        require(map.emplace(key, key * 2).second);
        if (map.bucket_count() != last_buckets) {
            last_buckets = map.bucket_count();
            ++rehashes;
        }
        require(map.load_factor() <= map.max_load_factor());
    }

    require(rehashes >= 2);
    require(contains_exactly(map, 0, growth_count));
    // The default policy uses power of two bucket counts.
    require(std::has_single_bit(map.bucket_count()));
}

void test_iteration_visits_each_element_once() {
    int_map map;
    fill_doubled(map, 0, growth_count);
    std::vector<int> seen(growth_count, 0);
    std::size_t visited = 0;

    for (const auto& [key, value] : map) {
        require_or_return(key >= 0, );
        require_or_return(key < growth_count, );
        require(value == key * 2);
        ++seen[static_cast<std::size_t>(key)];
        ++visited;
    }

    require(visited == map.size());
    require(std::ranges::all_of(seen, [](int times) {
        return times == 1;
    }));
}

void test_values_are_mutable_through_iterators() {
    int_map map;
    fill_doubled(map, 0, growth_count);

    for (auto& [key, value] : map) {
        value = key * 3;
    }

    require(map.at(1234) == 1234 * 3);
}

constexpr int erase_count = 2000;

void test_erase_by_key_returns_erased_count() {
    int_map map;
    fill_doubled(map, 0, erase_count);

    for (int key = 0; key < erase_count; key += 2) {
        require(map.erase(key) == 1);
    }

    require(map.erase(0) == 0);
    require(contains_exactly(map, 1, erase_count, 2));
}

void test_erased_keys_can_be_reinserted() {
    int_map map;
    fill_doubled(map, 0, erase_count);
    for (int key = 0; key < erase_count; key += 2) {
        map.erase(key);
    }

    fill_doubled(map, 0, erase_count, 2);

    require(contains_exactly(map, 0, erase_count));
}

void test_erase_by_iterator_removes_element() {
    // Erase and reinsert first so the table layout is reshuffled.
    int_map map;
    fill_doubled(map, 0, erase_count);
    for (int key = 0; key < erase_count; key += 2) {
        map.erase(key);
    }
    fill_doubled(map, 0, erase_count, 2);

    for (int key = 0; key < erase_count; key += 3) {
        auto iter = map.find(key);
        require_or_return(iter != map.end(), );
        map.erase(iter);
    }

    for (int key = 0; key < erase_count; ++key) {
        require(map.count(key) == (key % 3 == 0 ? 0U : 1U));
    }
}

void test_erasing_only_element_returns_end() {
    int_map map{
        { 7, 14 }
    };

    const int_map::iterator next = map.erase(map.begin());

    require(next == map.end());
    require(map.empty());
    require(map.emplace(7, 14).second);
    require(map.size() == 1);
}

void test_erase_while_iterating_visits_every_element() {
    constexpr int count = 1000;
    int_map map;
    fill_doubled(map, 0, count);

    for (auto iter = map.begin(); iter != map.end();) {
        if (iter->first % 2 == 0) {
            iter = map.erase(iter);
        } else {
            ++iter;
        }
    }

    require(contains_exactly(map, 1, count, 2));
}

constexpr int range_erase_count = 300;

void test_erase_full_range_empties_map() {
    int_map map;
    fill_doubled(map, 0, range_erase_count);

    map.erase(map.cbegin(), map.cend());

    require(map.empty());
    require(map.begin() == map.end());
}

void test_erase_subrange_removes_exactly_that_range() {
    constexpr int offsets      = 50;
    constexpr int range_start  = 50;
    constexpr int range_length = 100;
    constexpr int key_stride   = 13;

    // Different key offsets place the subrange at different table positions.
    for (int offset = 0; offset < offsets; ++offset) {
        int_map map;
        for (int key = 0; key < range_erase_count; ++key) {
            map.emplace(key * key_stride + offset, key);
        }
        auto first = std::next(map.begin(), range_start);
        auto last  = std::next(first, range_length);
        std::set<int> erased;
        for (auto iter = first; iter != last; ++iter) {
            erased.insert(iter->first);
        }

        auto empty_range = map.erase(first, first);
        map.erase(first, last);

        require(empty_range == first);
        require(map.size() == range_erase_count - erased.size());
        for (int key = 0; key < range_erase_count; ++key) {
            const int actual = key * key_stride + offset;
            require(map.count(actual) == (erased.contains(actual) ? 0U : 1U));
        }
    }
}

void test_string_elements_survive_erase() {
    const string_map map = make_long_string_map();

    for (int i = 0; i < long_string_count; ++i) {
        auto iter = map.find(long_string(i));
        require((iter == map.end()) == (i % 2 == 0));
        if (iter != map.end()) {
            require(iter->second == long_string(i * 2));
        }
    }
}

void test_emplace_and_at_with_convertible_key() {
    string_map map = make_long_string_map();

    map.emplace("short", "value");

    require(map.at("short") == "value");
    require(map.size() == long_string_count / 2 + 1);
}

void test_copy_is_independent_of_source() {
    const string_map source = make_string_source();

    string_map copy(source);
    require(copy == source);
    require(copy.size() == string_source_size);
    copy["0"] = "changed";

    require(copy != source);
    require(source.at("0") == std::string(30, 'v'));
}

void test_copy_assignment_replaces_contents() {
    const string_map source = make_string_source();
    string_map assigned;
    assigned["stale"] = "stale";

    assigned = source;

    require(assigned == source);
    require(assigned.count("stale") == 0);
}

void test_self_assignment_keeps_contents() {
    const string_map source = make_string_source();
    string_map assigned     = source;
    auto& alias             = assigned;

    assigned = alias;
    require(assigned == source);
    assigned = std::move(alias);

    require(assigned == source);
}

void test_moved_from_map_is_empty_and_reusable() {
    string_map source = make_string_source();

    string_map moved(std::move(source));

    // NOLINTBEGIN(bugprone-use-after-move)
    require(moved.size() == string_source_size);
    require(source.empty());
    require(source.begin() == source.end());
    source["reuse"] = "ok";
    require(source.size() == 1);
    require(source.at("reuse") == "ok");
    // NOLINTEND(bugprone-use-after-move)
}

void test_move_assignment_replaces_contents() {
    string_map source = make_string_source();
    source["0"]       = "changed";
    string_map move_assigned;
    move_assigned["stale"] = "stale";

    move_assigned = std::move(source);

    require(move_assigned.size() == string_source_size);
    require(move_assigned.count("stale") == 0);
    require(move_assigned.at("0") == "changed");
}

void test_copy_of_empty_map_is_empty() {
    const string_map empty;

    const string_map copy(empty);

    require(copy.empty());
    require(copy == empty);
}

void test_member_swap_exchanges_contents_and_load_factor() {
    int_map lhs;
    int_map rhs;
    fill_doubled(lhs, 0, 100);
    fill_doubled(rhs, 1000, 1010);
    lhs.max_load_factor(0.5F);

    lhs.swap(rhs);

    require(contains_exactly(lhs, 1000, 1010));
    require(contains_exactly(rhs, 0, 100));
    require(rhs.max_load_factor() == 0.5F);
}

void test_adl_swap_exchanges_contents() {
    int_map lhs;
    int_map rhs;
    fill_doubled(lhs, 1000, 1010);
    fill_doubled(rhs, 0, 100);

    using std::swap;
    swap(lhs, rhs);

    require(contains_exactly(lhs, 0, 100));
    require(contains_exactly(rhs, 1000, 1010));
}

void test_equality_ignores_insertion_order_and_bucket_count() {
    const auto [lhs, rhs] = make_equal_maps();

    require(lhs == rhs);
    require_false(lhs != rhs);
}

void test_maps_with_different_value_are_unequal() {
    auto [lhs, rhs] = make_equal_maps();

    rhs[0] = 1;

    require_false(lhs == rhs);
}

void test_maps_with_different_size_are_unequal() {
    auto [lhs, rhs] = make_equal_maps();

    rhs.erase(0);

    require(lhs != rhs);
}

constexpr int reserve_count = 1000;

void test_reserve_prevents_rehash_while_filling() {
    int_map map;

    map.reserve(reserve_count);
    const std::size_t reserved = map.bucket_count();
    fill_doubled(map, 0, reserve_count);

    require(reserved >= reserve_count);
    require(map.bucket_count() == reserved);
}

void test_smaller_reserve_and_rehash_keep_elements() {
    int_map map;
    map.reserve(reserve_count);
    fill_doubled(map, 0, reserve_count);

    map.reserve(1);
    map.rehash(1);

    require(contains_exactly(map, 0, reserve_count));
    require(map.load_factor() <= map.max_load_factor());
}

void test_rehash_grows_bucket_count() {
    int_map map;
    map.reserve(reserve_count);
    const std::size_t reserved = map.bucket_count();
    fill_doubled(map, 0, reserve_count);

    map.rehash(reserved * 4);

    require(map.bucket_count() >= reserved * 4);
    require(contains_exactly(map, 0, reserve_count));
}

void test_shrink_to_fit_after_erase_reduces_buckets() {
    constexpr int kept = 10;
    int_map map;
    map.reserve(reserve_count);
    const std::size_t reserved = map.bucket_count();
    fill_doubled(map, 0, reserve_count);
    map.rehash(reserved * 4);

    for (int key = kept; key < reserve_count; ++key) {
        map.erase(key);
    }
    map.shrink_to_fit();

    require(map.bucket_count() < reserved);
    require(contains_exactly(map, 0, kept));
}

void test_clear_and_shrink_release_all_buckets() {
    constexpr int count = 10;
    int_map map;
    fill_doubled(map, 0, count);

    map.clear();
    map.shrink_to_fit();

    require(map.bucket_count() == 0);
    require(map.empty());
    map.rehash(0);
    fill_doubled(map, 0, count);
    require(contains_exactly(map, 0, count));
}

void test_lower_max_load_factor_is_respected() {
    constexpr float max_load = 0.25F;
    int_map sparse;

    sparse.max_load_factor(max_load);
    fill_doubled(sparse, 0, reserve_count);

    require(sparse.max_load_factor() == max_load);
    require(sparse.load_factor() <= max_load);
    require(contains_exactly(sparse, 0, reserve_count));
}

/// Inserts, erases, reinserts and rehashes under the policy of `Hash`.
template <typename Hash>
void check_policy_round_trip() {
    constexpr int count = 5000;
    bytell_hash_map<int, int, Hash> map;

    fill_doubled(map, 0, count);
    require(contains_exactly(map, 0, count));
    for (int key = 0; key < count; key += 2) {
        map.erase(key);
    }
    require(contains_exactly(map, 1, count, 2));
    fill_doubled(map, 0, count, 2);
    map.rehash(count * 3);

    require(contains_exactly(map, 0, count));
}

void test_power_of_two_policy_round_trip() {
    check_policy_round_trip<neutron::power_of_two_std_hash<int>>();
}

void test_prime_number_policy_round_trip() {
    check_policy_round_trip<prime_hash>();
}

/// Inserts, erases and reinserts keys whose hashes collide under `Hash`.
template <typename Hash>
void check_collisions(int count) {
    bytell_hash_map<int, int, Hash> map;

    fill_doubled(map, 0, count);
    require(contains_exactly(map, 0, count));
    // Erasing chain heads and middles relinks the chain.
    for (int key = 0; key < count; key += 2) {
        require(map.erase(key) == 1);
    }
    require(contains_exactly(map, 1, count, 2));
    fill_doubled(map, 0, count, 2);

    require(contains_exactly(map, 0, count));
}

void test_constant_hash_collisions_keep_all_elements() {
    check_collisions<constant_hash>(200);
}

void test_coarse_hash_collisions_keep_all_elements() {
    check_collisions<coarse_hash>(300);
}

void test_set_operations_destroy_every_element_once() {
    constexpr int count = 1000;
    reset_counted();

    {
        counted_set set;
        for (int i = 0; i < count; ++i) {
            set.emplace(i);
        }
        require(counted::live == count);
        for (int i = 0; i < count; i += 3) {
            set.erase(counted{ i });
        }
        auto copy = set;
        set.clear();
        require(set.empty());
        require(counted::live == static_cast<int>(copy.size()));
        auto moved = std::move(copy);
        set        = moved;
        set.rehash(4096);
        set.shrink_to_fit();
        moved = std::move(set);
    }

    require(counted_clean());
}

void test_map_range_erase_destroys_values() {
    constexpr int count  = 500;
    constexpr int erased = 100;
    reset_counted();

    {
        bytell_hash_map<int, counted> map;
        for (int i = 0; i < count; ++i) {
            map[i].value = i;
        }
        map.erase(map.begin(), std::next(map.begin(), erased));
        require(counted::live == count - erased);
        require(map.size() == count - erased);
    }

    require(counted_clean());
}

constexpr int counted_source_size = 100;

void test_throwing_copy_construction_rolls_back() {
    reset_counted();

    {
        counted_set source;
        for (int i = 0; i < counted_source_size; ++i) {
            source.emplace(i);
        }
        counted::throw_on_copy = 50;
        bool thrown            = false;

        ATOM_TRY {
            counted_set copy(source);
            static_cast<void>(copy);
        }
        ATOM_CATCH(const std::runtime_error&) { thrown = true; }
        counted::throw_on_copy = -1;

        require(thrown);
        require(counted::live == counted_source_size);
        require(source.size() == counted_source_size);
    }

    require(counted_clean());
}

void test_throwing_insert_leaves_set_unchanged() {
    reset_counted();

    {
        counted_set source;
        for (int i = 0; i < counted_source_size; ++i) {
            source.emplace(i);
        }
        const counted value{ 1000 };
        counted::throw_on_copy = 0;
        bool thrown            = false;

        ATOM_TRY { source.insert(value); }
        ATOM_CATCH(const std::runtime_error&) { thrown = true; }
        counted::throw_on_copy = -1;

        require(thrown);
        require(source.size() == counted_source_size);
        require(source.count(value) == 0);
        // The source elements plus `value`.
        require(counted::live == counted_source_size + 1);
        for (int i = 0; i < counted_source_size; ++i) {
            require(source.count(counted{ i }) == 1);
        }
    }

    require(counted_clean());
}

void test_move_only_values_survive_rehash_and_move() {
    constexpr int count = 500;
    bytell_hash_map<int, std::unique_ptr<int>> map;
    for (int i = 0; i < count; ++i) {
        map.emplace(i, std::make_unique<int>(i));
    }

    map[1000] = std::make_unique<int>(1000);
    map.rehash(4096);
    for (int i = 0; i < count; ++i) {
        require(*map.at(i) == i);
    }
    auto moved = std::move(map);

    require(moved.size() == count + 1);
    require(*moved.at(1000) == 1000);
}

void test_pair_elements_are_suitably_aligned() {
    constexpr int count = 1000;
    bytell_hash_map<long long, double> map;

    for (int i = 0; i < count; ++i) {
        map.emplace(i, i * 0.5);
    }

    for (const auto& [key, value] : map) {
        const auto address = reinterpret_cast<std::uintptr_t>(&key);
        require(address % alignof(std::pair<long long, double>) == 0);
        require(value == static_cast<double>(key) * 0.5);
    }
    require(map.size() == count);
}

void test_lookup_in_empty_over_aligned_set_finds_nothing() {
    // Empty tables share a static block, which must suit any alignment.
    const bytell_hash_set<over_aligned, over_aligned_hash> set;

    require(set.find(over_aligned{ 1 }) == set.end());
}

void test_over_aligned_elements_are_aligned() {
    constexpr int count = 256;
    bytell_hash_set<over_aligned, over_aligned_hash> set;

    for (int i = 0; i < count; ++i) {
        set.insert(over_aligned{ i });
    }

    for (const over_aligned& item : set) {
        const auto address = reinterpret_cast<std::uintptr_t>(&item);
        require(address % alignof(over_aligned) == 0);
    }
    require(set.size() == count);
}

void test_map_allocates_from_its_allocator() {
    arena first;

    {
        const tracked_map map = make_tracked_map(first);
        require(map.get_allocator().owner == &first);
        require(first.blocks.size() == 1);
    }

    require(arena_clean(first));
}

void test_copy_keeps_allocator_and_extended_copy_uses_given_one() {
    arena first;
    arena second;

    {
        const tracked_map map = make_tracked_map(first);

        const tracked_map copy(map);
        const tracked_map other(map, tracked_alloc{ &second });

        require(copy.get_allocator().owner == &first);
        require(other.get_allocator().owner == &second);
        require(copy == map);
        require(other == map);
        require(second.blocks.size() == 1);
    }

    require(arena_clean(first));
    require(arena_clean(second));
}

void test_extended_move_with_unequal_allocator_moves_elements() {
    arena first;
    arena second;

    {
        const tracked_map map = make_tracked_map(first);
        tracked_map source(map, tracked_alloc{ &first });

        const tracked_map moved(std::move(source), tracked_alloc{ &second });

        require(moved.get_allocator().owner == &second);
        require(moved == map);
        // NOLINTNEXTLINE(bugprone-use-after-move)
        require(source.empty());
    }

    require(arena_clean(first));
    require(arena_clean(second));
}

void test_extended_move_with_equal_allocator_steals_storage() {
    arena first;
    arena second;

    {
        const tracked_map map = make_tracked_map(first);
        tracked_map source(map, tracked_alloc{ &second });
        const std::size_t second_blocks = second.blocks.size();

        const tracked_map stolen(std::move(source), tracked_alloc{ &second });

        require(stolen == map);
        require(second.blocks.size() == second_blocks);
    }

    require(arena_clean(first));
    require(arena_clean(second));
}

void test_move_assignment_with_unequal_allocator_moves_elements() {
    arena first;
    arena second;

    {
        const tracked_map map = make_tracked_map(first);
        tracked_map copy(map);
        tracked_map other(map, tracked_alloc{ &second });

        other = std::move(copy);

        require(other.get_allocator().owner == &second);
        require(other == map);
    }

    require(arena_clean(first));
    require(arena_clean(second));
}

void test_copy_assignment_keeps_target_allocator() {
    arena first;
    arena second;

    {
        const tracked_map map = make_tracked_map(first);
        tracked_map target{ tracked_alloc{ &second } };

        target = map;

        require(target.get_allocator().owner == &second);
        require(target == map);
    }

    require(arena_clean(first));
    require(arena_clean(second));
}

void test_move_assignment_with_equal_allocator_steals_storage() {
    arena first;

    {
        tracked_map map = make_tracked_map(first);
        tracked_map stolen{ tracked_alloc{ &first } };
        const std::size_t blocks = first.blocks.size();

        stolen = std::move(map);

        require(stolen.size() == tracked_size);
        require(first.blocks.size() <= blocks);
    }

    require(arena_clean(first));
}

constexpr int propagating_size = 100;

/// Fills a propagating set with `[first, first + propagating_size)`.
void fill_propagating(propagating_set& set, int first) {
    for (int i = 0; i < propagating_size; ++i) {
        set.insert(first + i);
    }
}

void test_propagating_copy_assignment_adopts_source_allocator() {
    arena first;
    arena second;

    {
        propagating_set source{ propagating_alloc{ &first } };
        fill_propagating(source, 0);
        propagating_set copied{ propagating_alloc{ &second } };
        copied.insert(-1);

        copied = source;

        require(copied.get_allocator().owner == &first);
        require(copied == source);
    }

    require(arena_clean(first));
    require(arena_clean(second));
}

void test_propagating_move_assignment_adopts_source_allocator() {
    arena first;
    arena second;

    {
        propagating_set source{ propagating_alloc{ &second } };
        fill_propagating(source, 1000);
        propagating_set moved{ propagating_alloc{ &first } };
        moved.insert(-1);

        moved = std::move(source);

        require(moved.get_allocator().owner == &second);
        require(moved.size() == propagating_size);
        require(moved.count(1000) == 1);
    }

    require(arena_clean(first));
    require(arena_clean(second));
}

void test_propagating_swap_exchanges_allocators() {
    arena first;
    arena second;

    {
        propagating_set lhs{ propagating_alloc{ &first } };
        propagating_set rhs{ propagating_alloc{ &second } };
        fill_propagating(lhs, 0);
        fill_propagating(rhs, 1000);

        lhs.swap(rhs);

        require(lhs.get_allocator().owner == &second);
        require(rhs.get_allocator().owner == &first);
        require(lhs.count(1000) == 1);
        require(rhs.count(0) == 1);
    }

    require(arena_clean(first));
    require(arena_clean(second));
}

void test_set_emplace_constructs_from_arguments() {
    bytell_hash_set<std::string> set;
    const std::string ccc = "ccc";
    std::string ddd       = "ddd";

    require(set.emplace(3, 'a').second);
    require(set.emplace("bbb").second);
    require(set.emplace(ccc).second);
    require(set.emplace(ddd).second);
    require(set.emplace(std::string("eee")).second);
    require_false(set.emplace("aaa").second);

    require(set.size() == 5);
    require(set.count("aaa") == 1);
    require(set.count("zzz") == 0);
}

void test_set_equality_ignores_insertion_order() {
    const bytell_hash_set<std::string> set{ "aaa", "bbb", "ccc", "ddd", "eee" };
    bytell_hash_set<std::string> other{ "eee", "ddd", "ccc", "bbb", "aaa" };

    require(set == other);
    other.erase("aaa");

    require(set != other);
}

void test_set_iteration_visits_unique_elements() {
    const bytell_hash_set<std::string> set{ "aaa", "bbb", "ccc", "ddd", "eee" };

    const std::set<std::string> visited(set.begin(), set.end());

    require(visited.size() == set.size());
}

void test_set_erase_removes_only_erased_keys() {
    constexpr int count = 3000;
    int_set numbers;
    for (int i = 0; i < count; ++i) {
        numbers.insert(i);
    }

    for (int i = 0; i < count; i += 2) {
        numbers.erase(i);
    }

    for (int i = 0; i < count; ++i) {
        require(numbers.count(i) == static_cast<std::size_t>(i % 2));
    }
    require(numbers.size() == count / 2);
}

} // namespace

int main() {
    test_default_constructed_map_is_empty();
    test_lookup_in_empty_map_finds_nothing();
    test_clear_and_shrink_on_empty_map_keep_it_empty();
    test_insert_returns_inserted_element();
    test_emplace_keeps_existing_value_for_duplicate_key();
    test_insert_lvalue_rejects_duplicate_key();
    test_hinted_insert_returns_element();
    test_subscript_inserts_default_value();
    test_emplace_without_arguments_inserts_default_pair();
    test_equal_range_finds_single_element();
    test_equal_range_of_missing_key_is_end();
    test_const_equal_range_finds_single_element();
    test_count_at_and_bucket_on_populated_map();
    test_at_throws_out_of_range_for_missing_key();
    test_insert_or_assign_inserts_then_assigns();
    test_hinted_insert_or_assign_inserts_and_assigns();
    test_range_constructor_keeps_first_duplicate();
    test_initializer_list_constructor_matches_range_constructor();
    test_bucket_count_constructor_reserves_buckets();
    test_initializer_list_with_bucket_count_constructor();
    test_set_initializer_list_drops_duplicates();
    test_insert_grows_within_max_load_factor();
    test_iteration_visits_each_element_once();
    test_values_are_mutable_through_iterators();
    test_erase_by_key_returns_erased_count();
    test_erased_keys_can_be_reinserted();
    test_erase_by_iterator_removes_element();
    test_erasing_only_element_returns_end();
    test_erase_while_iterating_visits_every_element();
    test_erase_full_range_empties_map();
    test_erase_subrange_removes_exactly_that_range();
    test_string_elements_survive_erase();
    test_emplace_and_at_with_convertible_key();
    test_copy_is_independent_of_source();
    test_copy_assignment_replaces_contents();
    test_self_assignment_keeps_contents();
    test_moved_from_map_is_empty_and_reusable();
    test_move_assignment_replaces_contents();
    test_copy_of_empty_map_is_empty();
    test_member_swap_exchanges_contents_and_load_factor();
    test_adl_swap_exchanges_contents();
    test_equality_ignores_insertion_order_and_bucket_count();
    test_maps_with_different_value_are_unequal();
    test_maps_with_different_size_are_unequal();
    test_reserve_prevents_rehash_while_filling();
    test_smaller_reserve_and_rehash_keep_elements();
    test_rehash_grows_bucket_count();
    test_shrink_to_fit_after_erase_reduces_buckets();
    test_clear_and_shrink_release_all_buckets();
    test_lower_max_load_factor_is_respected();
    test_power_of_two_policy_round_trip();
    test_prime_number_policy_round_trip();
    test_constant_hash_collisions_keep_all_elements();
    test_coarse_hash_collisions_keep_all_elements();
    test_set_operations_destroy_every_element_once();
    test_map_range_erase_destroys_values();
    test_throwing_copy_construction_rolls_back();
    test_throwing_insert_leaves_set_unchanged();
    test_move_only_values_survive_rehash_and_move();
    test_pair_elements_are_suitably_aligned();
    test_lookup_in_empty_over_aligned_set_finds_nothing();
    test_over_aligned_elements_are_aligned();
    test_map_allocates_from_its_allocator();
    test_copy_keeps_allocator_and_extended_copy_uses_given_one();
    test_extended_move_with_unequal_allocator_moves_elements();
    test_extended_move_with_equal_allocator_steals_storage();
    test_move_assignment_with_unequal_allocator_moves_elements();
    test_copy_assignment_keeps_target_allocator();
    test_move_assignment_with_equal_allocator_steals_storage();
    test_propagating_copy_assignment_adopts_source_allocator();
    test_propagating_move_assignment_adopts_source_allocator();
    test_propagating_swap_exchanges_allocators();
    test_set_emplace_constructs_from_arguments();
    test_set_equality_ignores_insertion_order();
    test_set_iteration_visits_unique_elements();
    test_set_erase_removes_only_erased_keys();
    return neutron_test::result();
}
