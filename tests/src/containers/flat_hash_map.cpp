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

namespace {

using int_map    = neutron::flat_hash_map<int, int>;
using string_map = neutron::flat_hash_map<std::string, std::string>;
using int_set    = neutron::flat_hash_set<int>;
using string_set = neutron::flat_hash_set<std::string>;

static_assert(std::same_as<int_map::key_type, int>);
static_assert(std::same_as<int_map::mapped_type, int>);
static_assert(std::same_as<int_map::value_type, std::pair<int, int>>);
static_assert(std::same_as<int_set::key_type, int>);
static_assert(std::same_as<int_set::value_type, int>);

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

static_assert(std::is_nothrow_move_constructible_v<int_map>);
static_assert(std::is_copy_constructible_v<string_map>);
static_assert(std::is_copy_assignable_v<string_map>);
static_assert(std::is_move_assignable_v<string_map>);

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

struct prime_hash : std::hash<int> {
    using hash_policy = neutron::prime_number_hash_policy;
};

/// Stateful hasher and equality to check they are stored and copied.
struct salted_hash {
    std::size_t salt = 0;
    std::size_t operator()(int key) const noexcept {
        return static_cast<std::size_t>(key) ^ salt;
    }
};

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

/// Allocator with an identity; tracks outstanding element counts per id.
inline long long outstanding[8];
inline long long allocations[8];

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

bool allocators_balanced() {
    return std::ranges::all_of(outstanding, [](long long val) {
        return val == 0;
    });
}

template <bool Pocca, bool Pocma, bool Pocs>
using id_map = neutron::flat_hash_map<
    int, std::string, std::hash<int>, std::equal_to<int>,
    id_allocator<std::pair<int, std::string>, Pocca, Pocma, Pocs>>;

template <typename Value>
Value value_for(int key) {
    if constexpr (std::same_as<Value, std::string>) {
        return std::to_string(key * 2);
    } else {
        return static_cast<Value>(key * 2);
    }
}

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

template <typename Map>
void fill(Map& map, int first, int last) {
    for (int i = first; i != last; ++i) {
        map.emplace(i, value_for<typename Map::mapped_type>(i));
    }
}

template <typename Range>
std::size_t distance_of(const Range& range) {
    return static_cast<std::size_t>(
        std::distance(std::ranges::begin(range), std::ranges::end(range)));
}

bool test_default_state() {
    int_map map;
    const auto& cmap = map;
    if (!map.empty() || map.size() != 0 || map.bucket_count() != 0) {
        return false;
    }
    if (map.begin() != map.end() || cmap.cbegin() != cmap.cend()) {
        return false;
    }
    if (map.find(1) != map.end() || cmap.count(1) != 0 || map.erase(1) != 0) {
        return false;
    }
    if (map.load_factor() != 0.0F || map.max_load_factor() != 0.5F) {
        return false;
    }
    auto range = map.equal_range(1);
    if (range.first != map.end() || range.second != map.end()) {
        return false;
    }
    map.clear();
    map.shrink_to_fit();
    map.rehash(0);
    return map.empty() && map.begin() == map.end();
}

bool test_insert_and_lookup() {
    int_map map;
    auto [iter, inserted] = map.insert({ 1, 10 });
    if (!inserted || iter->first != 1 || iter->second != 10) {
        return false;
    }
    // Duplicates are rejected and keep the old value.
    auto dup = map.insert({ 1, 99 });
    if (dup.second || dup.first != iter || dup.first->second != 10) {
        return false;
    }
    if (!map.emplace(2, 20).second || map.emplace(2, 21).second) {
        return false;
    }
    std::pair<int, int> value{ 3, 30 };
    if (!map.insert(value).second || map.insert(std::move(value)).second) {
        return false;
    }
    map[4]  = 40;
    int key = 5;
    map[std::move(key)] += 50;
    if (map[4] != 40 || map[5] != 50 || map.size() != 5) {
        return false;
    }
    if (map.at(1) != 10 || std::as_const(map).at(2) != 20) {
        return false;
    }
    map.at(1) = 11;
    if (map.find(1)->second != 11) {
        return false;
    }
    // Default-constructed value from operator[] and empty emplace().
    if (map[6] != 0 || map.size() != 6) {
        return false;
    }
    auto [zero, zero_inserted] = map.emplace();
    if (!zero_inserted || zero->first != 0 || zero->second != 0) {
        return false;
    }
    if (map.count(3) != 1 || map.count(100) != 0) {
        return false;
    }
    auto range = map.equal_range(3);
    if (range.first == map.end() || range.first->first != 3 ||
        std::next(range.first) != range.second) {
        return false;
    }
    auto crange = std::as_const(map).equal_range(100);
    if (crange.first != map.end() || crange.second != map.end()) {
        return false;
    }
    return map.size() == 7 && distance_of(map) == 7;
}

bool test_hint_and_assign() {
    int_map map;
    auto iter = map.insert(map.cend(), { 1, 1 });
    if (iter->first != 1 || iter->second != 1) {
        return false;
    }
    const std::pair<int, int> value{ 2, 2 };
    if (map.insert(map.cbegin(), value)->second != 2) {
        return false;
    }
    if (map.emplace_hint(map.cend(), 3, 3)->second != 3) {
        return false;
    }
    if (map.emplace_hint(map.cend(), 3, 4)->second != 3) {
        return false;
    }

    auto assigned = map.insert_or_assign(1, 100);
    if (assigned.second || assigned.first->second != 100) {
        return false;
    }
    int key  = 4;
    assigned = map.insert_or_assign(std::move(key), 400);
    if (!assigned.second || assigned.first->second != 400) {
        return false;
    }
    if (map.insert_or_assign(map.cend(), 2, 200)->second != 200) {
        return false;
    }
    if (map.insert_or_assign(map.cend(), 5, 500)->second != 500) {
        return false;
    }

    map.insert(
        {
            { 6,  6 },
            { 7,  7 },
            { 6, 66 }
    });
    const std::vector<std::pair<int, int>> more{
        { 8, 8 },
        { 9, 9 }
    };
    map.insert(more.begin(), more.end());
    return map.size() == 9 && map.at(6) == 6 && map.at(9) == 9 &&
           map.at(1) == 100;
}

bool test_at_throws() {
    int_map map{
        { 1, 1 }
    };
    bool thrown = false;
    ATOM_TRY { static_cast<void>(map.at(2)); }
    ATOM_CATCH(const std::out_of_range&) { thrown = true; }
    if (!thrown) {
        return false;
    }
    thrown = false;
    ATOM_TRY { static_cast<void>(std::as_const(map).at(3)); }
    ATOM_CATCH(const std::out_of_range&) { thrown = true; }
    return thrown && map.size() == 1;
}

bool test_growth() {
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
        if (map.load_factor() > map.max_load_factor()) {
            return false;
        }
    }
    if (growths < 2 || map.size() != count || !contains_all(map, 0, count)) {
        return false;
    }
    // Fibonacci hashing uses power-of-two bucket counts.
    if (!std::has_single_bit(map.bucket_count())) {
        return false;
    }
    if (map.bucket(123) >= map.bucket_count()) {
        return false;
    }
    long long sum       = 0;
    std::size_t visited = 0;
    for (const auto& [key, value] : std::as_const(map)) {
        sum += key;
        ++visited;
        if (value != key * 2) {
            return false;
        }
    }
    return visited == count &&
           sum == static_cast<long long>(count) * (count - 1) / 2;
}

bool test_rehash_reserve_shrink() {
    int_map map;
    map.reserve(1000);
    const auto reserved = map.bucket_count();
    if (reserved < 2000 || !std::has_single_bit(reserved)) {
        return false;
    }
    fill(map, 0, 1000);
    if (map.bucket_count() != reserved || !contains_all(map, 0, 1000)) {
        return false;
    }

    map.rehash(10000);
    if (map.bucket_count() < 10000 || !contains_all(map, 0, 1000)) {
        return false;
    }
    // Rehash never drops below what the current size requires.
    map.rehash(1);
    if (map.bucket_count() < 2000 || !contains_all(map, 0, 1000)) {
        return false;
    }

    for (int i = 10; i != 1000; ++i) {
        if (map.erase(i) != 1) {
            return false;
        }
    }
    map.shrink_to_fit();
    if (map.size() != 10 || map.bucket_count() > 32 ||
        !contains_all(map, 0, 10)) {
        return false;
    }

    map.clear();
    const auto cleared = map.bucket_count();
    if (!map.empty() || cleared == 0 || map.begin() != map.end()) {
        return false;
    }
    map.shrink_to_fit();
    if (map.bucket_count() != 0) {
        return false;
    }
    fill(map, 0, 100);
    if (!contains_all(map, 0, 100)) {
        return false;
    }

    // A larger load factor needs fewer buckets.
    int_map dense;
    dense.max_load_factor(0.9F);
    if (dense.max_load_factor() != 0.9F) {
        return false;
    }
    fill(dense, 0, 900);
    int_map sparse;
    fill(sparse, 0, 900);
    return dense.load_factor() <= 0.9F &&
           dense.bucket_count() < sparse.bucket_count() &&
           contains_all(dense, 0, 900);
}

bool test_erase() {
    constexpr int count = 2000;

    int_map map;
    fill(map, 0, count);

    // Erase by key.
    for (int i = 0; i < count; i += 3) {
        if (map.erase(i) != 1 || map.erase(i) != 0) {
            return false;
        }
    }
    for (int i = 0; i != count; ++i) {
        const bool expected = (i % 3) != 0;
        if ((map.count(i) == 1) != expected) {
            return false;
        }
    }

    // Erase through the iterator returned by find.
    auto found = map.find(1);
    if (found == map.end()) {
        return false;
    }
    map.erase(found);
    if (map.count(1) != 0) {
        return false;
    }

    // Erase while iterating visits every element exactly once.
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
    if (visited != before || map.size() != before - removed) {
        return false;
    }
    for (const auto& [key, value] : map) {
        if (key % 2 == 0 || key % 3 == 0 || value != key * 2) {
            return false;
        }
    }

    // Reinsert everything after erasure.
    fill(map, 0, count);
    if (map.size() != count || !contains_all(map, 0, count)) {
        return false;
    }

    // Erase all, table stays usable.
    for (int i = 0; i != count; ++i) {
        map.erase(i);
    }
    if (!map.empty() || map.begin() != map.end()) {
        return false;
    }
    fill(map, 0, 10);
    return contains_all(map, 0, 10) && map.size() == 10;
}

template <typename Map>
bool check_range_erase(int count) {
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
            if (map.size() != size - (stop - start)) {
                return false;
            }
            if (to_end ? result != map.end()
                       : (result == map.end() || result->first != next_key)) {
                return false;
            }
            for (int i = 0; i != count; ++i) {
                const bool found =
                    map.find(i) != map.end() && map.find(i)->second == i * 2;
                if (found == erased[static_cast<std::size_t>(i)]) {
                    return false;
                }
            }
            if (distance_of(map) != map.size()) {
                return false;
            }
        }
    }
    return true;
}

bool test_range_erase() {
    if (!check_range_erase<int_map>(100)) {
        return false;
    }
    if (!check_range_erase<neutron::flat_hash_map<int, int, grouped_hash>>(
            100)) {
        return false;
    }
    int_map map;
    fill(map, 0, 50);
    if (map.erase(map.cbegin(), map.cend()) != map.end() || !map.empty()) {
        return false;
    }
    fill(map, 0, 50);
    return contains_all(map, 0, 50);
}

template <typename Map>
bool check_with_hash(int count) {
    Map map;
    fill(map, 0, count);
    if (map.size() != static_cast<std::size_t>(count) ||
        !contains_all(map, 0, count)) {
        return false;
    }
    // Remove odd keys so that backward shifting crosses clusters.
    for (int i = 1; i < count; i += 2) {
        if (map.erase(i) != 1) {
            return false;
        }
    }
    for (int i = 0; i != count; ++i) {
        if ((map.count(i) == 1) != (i % 2 == 0)) {
            return false;
        }
    }
    fill(map, 0, count);
    return contains_all(map, 0, count) &&
           distance_of(map) == static_cast<std::size_t>(count);
}

bool test_collisions() {
    if (!check_with_hash<neutron::flat_hash_map<int, int, grouped_hash>>(
            2000)) {
        return false;
    }
    // Every key lands in the same bucket.
    using colliding = neutron::flat_hash_map<int, int, constant_hash>;
    if (!check_with_hash<colliding>(6)) {
        return false;
    }
    colliding map;
    fill(map, 0, 6);
    map.erase(2);
    map.erase(0);
    return map.size() == 4 && contains_all(map, 3, 6) && map.count(1) == 1 &&
           map.count(0) == 0 && map.count(2) == 0;
}

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

bool test_hash_policies() {
    using pow2_map =
        neutron::flat_hash_map<int, int, neutron::power_of_two_std_hash<int>>;
    using prime_map = neutron::flat_hash_map<int, int, prime_hash>;
    if (!check_with_hash<pow2_map>(3000) || !check_with_hash<prime_map>(3000)) {
        return false;
    }
    pow2_map pow2;
    fill(pow2, 0, 1000);
    prime_map prime;
    fill(prime, 0, 1000);
    if (!std::has_single_bit(pow2.bucket_count()) ||
        !is_prime(prime.bucket_count())) {
        return false;
    }
    if (pow2.bucket(999) >= pow2.bucket_count() ||
        prime.bucket(999) >= prime.bucket_count()) {
        return false;
    }
    prime.clear();
    prime.shrink_to_fit();
    if (prime.bucket_count() != 0 || prime.find(1) != prime.end()) {
        return false;
    }
    fill(prime, 0, 10);
    return contains_all(prime, 0, 10);
}

bool test_copy_and_move() {
    string_map source;
    for (int i = 0; i != 200; ++i) {
        source.emplace(std::to_string(i), std::string(40, 'a' + (i % 26)));
    }

    string_map copy(source);
    if (copy != source || !(copy == source) || copy.size() != 200) {
        return false;
    }
    copy["0"] = "changed";
    if (copy == source || source["0"] != std::string(40, 'a')) {
        return false;
    }

    string_map assigned{
        { "x", "y" }
    };
    assigned = source;
    if (assigned != source || assigned.count("x") != 0) {
        return false;
    }
    auto& self = assigned;
    assigned   = self;
    if (assigned != source) {
        return false;
    }

    string_map moved(std::move(assigned));
    if (moved != source || !assigned.empty() ||
        assigned.begin() != assigned.end()) {
        return false;
    }
    assigned.emplace("reuse", "ok");
    if (assigned.size() != 1 || assigned.at("reuse") != "ok") {
        return false;
    }

    string_map move_assigned{
        { "old", "value" }
    };
    move_assigned = std::move(moved);
    if (move_assigned != source || move_assigned.count("old") != 0 ||
        !moved.empty()) {
        return false;
    }
    auto& self_move = move_assigned;
    move_assigned   = std::move(self_move);
    if (move_assigned != source) {
        return false;
    }

    string_map other{
        { "a", "b" }
    };
    other.swap(move_assigned);
    if (other != source || move_assigned.size() != 1 ||
        move_assigned.at("a") != "b") {
        return false;
    }
    std::swap(other, move_assigned);
    if (move_assigned != source || other.at("a") != "b") {
        return false;
    }

    // Copies of empty maps.
    const string_map empty;
    string_map empty_copy(empty);
    string_map empty_assigned{
        { "k", "v" }
    };
    empty_assigned = empty;
    return empty_copy.empty() && empty_assigned.empty() &&
           empty_copy == empty &&
           empty_assigned.begin() == empty_assigned.end();
}

bool test_equality() {
    int_map lhs;
    int_map rhs;
    rhs.reserve(1000);
    for (int i = 0; i != 100; ++i) {
        lhs.emplace(i, i);
        rhs.emplace(99 - i, 99 - i);
    }
    if (lhs != rhs || !(lhs == rhs)) {
        return false;
    }
    rhs[50] = -1;
    if (lhs == rhs) {
        return false;
    }
    rhs.erase(50);
    return lhs != rhs && int_map{} == int_map{};
}

bool test_constructors() {
    const std::vector<std::pair<int, int>> values{
        { 1,   2 },
        { 2,   4 },
        { 3,   6 },
        { 1, 100 }
    };
    int_map from_range(values.begin(), values.end());
    if (from_range.size() != 3 || from_range.at(1) != 2 ||
        !contains_all(from_range, 1, 4)) {
        return false;
    }
    int_map from_list{
        { 1, 2 },
        { 2, 4 },
        { 3, 6 }
    };
    if (from_list != from_range || from_list.bucket_count() == 0) {
        return false;
    }
    int_map with_buckets(100);
    if (with_buckets.bucket_count() < 100 || !with_buckets.empty()) {
        return false;
    }
    int_map list_buckets(
        {
            { 1, 2 }
    },
        64);
    return list_buckets.bucket_count() >= 64 && list_buckets.at(1) == 2;
}

bool test_string_keys() {
    string_map map;
    map.emplace("alpha", "1");
    map.emplace(std::string("beta"), "2");
    map.emplace(std::make_pair("gamma", "3"));
    map.insert(std::make_pair(std::string("delta"), std::string("4")));
    map["epsilon"]      = "5";
    std::string key     = "zeta";
    map[std::move(key)] = "6";
    if (map.size() != 6 || map.at("alpha") != "1" || map.at("gamma") != "3" ||
        map.at("zeta") != "6") {
        return false;
    }
    if (map.emplace("alpha", "x").second ||
        map.emplace(std::make_pair("gamma", "y")).second) {
        return false;
    }
    // Long strings exercise moves of heap buffers during rehash.
    for (int i = 0; i != 500; ++i) {
        map.emplace(
            std::string(64, 'k') + std::to_string(i), std::to_string(i));
    }
    for (int i = 0; i != 500; ++i) {
        auto iter = map.find(std::string(64, 'k') + std::to_string(i));
        if (iter == map.end() || iter->second != std::to_string(i)) {
            return false;
        }
    }
    return map.erase("alpha") == 1 && map.count("alpha") == 0 &&
           map.size() == 505;
}

bool test_object_lifetime() {
    {
        neutron::flat_hash_map<tracked, tracked, tracked_hash> map;
        for (int i = 0; i != 1000; ++i) {
            map.emplace(tracked{ i }, tracked{ i * 2 });
        }
        for (int i = 0; i < 1000; i += 2) {
            map.erase(tracked{ i });
        }
        map[tracked{ 5000 }].value = 7;
        auto copy                  = map;
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
        if (!map.empty() || moved.size() != 501 ||
            moved.at(tracked{ 5000 }).value != 7 ||
            moved.at(tracked{ 1 }).value != 2) {
            return false;
        }

        neutron::flat_hash_set<tracked, tracked_hash> set;
        for (int i = 0; i != 300; ++i) {
            set.emplace(i);
            set.insert(tracked{ i });
        }
        auto set_copy = set;
        for (int i = 0; i < 300; i += 3) {
            set_copy.erase(tracked{ i });
        }
        if (set.size() != 300 || set_copy.size() != 200) {
            return false;
        }
    }
    return tracked::live == 0 && tracked::errors == 0;
}

bool test_move_only() {
    neutron::flat_hash_map<int, std::unique_ptr<int>> map;
    for (int i = 0; i != 300; ++i) {
        map.emplace(i, std::make_unique<int>(i));
    }
    map[1000]  = std::make_unique<int>(1000);
    auto moved = std::move(map);
    neutron::flat_hash_map<int, std::unique_ptr<int>> target;
    target = std::move(moved);
    if (target.size() != 301 || *target.at(1000) != 1000) {
        return false;
    }
    for (int i = 0; i != 300; ++i) {
        auto iter = target.find(i);
        if (iter == target.end() || !iter->second || *iter->second != i) {
            return false;
        }
    }
    neutron::flat_hash_set<std::unique_ptr<int>> set;
    auto ptr  = std::make_unique<int>(1);
    auto* raw = ptr.get();
    set.insert(std::move(ptr));
    return set.size() == 1 && set.begin()->get() == raw;
}

bool test_exception_safety() {
    using map = neutron::flat_hash_map<int, throwing_copy>;
    {
        map source;
        for (int i = 0; i != 100; ++i) {
            source.emplace(i, throwing_copy{ i });
        }
        const int baseline = throwing_copy::live;

        // Copy construction fails midway and releases everything.
        throwing_copy::budget = 50;
        bool thrown           = false;
        ATOM_TRY {
            map copy(source);
            static_cast<void>(copy);
        }
        ATOM_CATCH(const std::runtime_error&) { thrown = true; }
        throwing_copy::budget = -1;
        if (!thrown || throwing_copy::live != baseline) {
            return false;
        }

        // Failed insertion keeps the existing contents.
        const throwing_copy value{ 1000 };
        const std::pair<int, throwing_copy> entry{ 1000, value };
        throwing_copy::budget = 0;
        thrown                = false;
        ATOM_TRY { source.insert(entry); }
        ATOM_CATCH(const std::runtime_error&) { thrown = true; }
        throwing_copy::budget = -1;
        if (!thrown || source.size() != 100 || source.count(1000) != 0) {
            return false;
        }
        for (int i = 0; i != 100; ++i) {
            if (source.at(i).value != i) {
                return false;
            }
        }

        // Failed copy assignment leaves a valid map.
        map target{
            { 1, throwing_copy{ 1 } }
        };
        throwing_copy::budget = 10;
        thrown                = false;
        ATOM_TRY { target = source; }
        ATOM_CATCH(const std::runtime_error&) { thrown = true; }
        throwing_copy::budget = -1;
        if (!thrown || target.size() > source.size() ||
            distance_of(target) != target.size()) {
            return false;
        }
        target = source;
        if (target != source) {
            return false;
        }
    }
    return throwing_copy::live == 0;
}

bool test_allocator() {
    {
        using map = id_map<false, false, false>;
        using alloc_type =
            id_allocator<std::pair<int, std::string>, false, false, false>;

        map first(alloc_type{ 1 });
        fill(first, 0, 100);
        if (first.get_allocator().id != 1 || outstanding[1] == 0 ||
            outstanding[2] != 0 || !contains_all(first, 0, 100)) {
            return false;
        }
        const alloc_type rebound = first.get_allocator();
        if (rebound.id != 1) {
            return false;
        }

        map copy(first);
        map copy_other(first, alloc_type{ 2 });
        if (copy.get_allocator().id != 1 ||
            copy_other.get_allocator().id != 2 || outstanding[2] == 0 ||
            copy_other != first || copy != first) {
            return false;
        }

        map moved(std::move(copy));
        if (moved.get_allocator().id != 1 || moved != first) {
            return false;
        }

        // Unequal allocators without propagation move element-wise.
        map target(alloc_type{ 3 });
        target.emplace(-1, "x");
        target = std::move(moved);
        if (target.get_allocator().id != 3 || target != first ||
            !moved.empty() || moved.get_allocator().id != 1) {
            return false;
        }
        // Equal allocators without propagation steal the buffer.
        map same(alloc_type{ 3 });
        const auto before = allocations[3];
        same              = std::move(target);
        if (same != first || allocations[3] != before) {
            return false;
        }

        // Copy assignment keeps its own allocator.
        map assigned(alloc_type{ 2 });
        assigned = first;
        if (assigned.get_allocator().id != 2 || assigned != first) {
            return false;
        }
    }
    if (!allocators_balanced()) {
        return false;
    }
    {
        using map = id_map<true, true, true>;
        using alloc_type =
            id_allocator<std::pair<int, std::string>, true, true, true>;

        map first(alloc_type{ 4 });
        fill(first, 0, 50);
        map second(alloc_type{ 5 });
        fill(second, 100, 120);

        map copy_assigned(alloc_type{ 6 });
        fill(copy_assigned, 0, 10);
        copy_assigned = first;
        if (copy_assigned.get_allocator().id != 4 || copy_assigned != first ||
            outstanding[6] != 0) {
            return false;
        }

        map move_assigned(alloc_type{ 6 });
        fill(move_assigned, 0, 10);
        move_assigned = std::move(copy_assigned);
        if (move_assigned.get_allocator().id != 4 || move_assigned != first ||
            outstanding[6] != 0) {
            return false;
        }

        first.swap(second);
        if (first.get_allocator().id != 5 || second.get_allocator().id != 4 ||
            !contains_all(first, 100, 120) || !contains_all(second, 0, 50)) {
            return false;
        }
    }
    return allocators_balanced();
}

bool test_stateful_functors() {
    using map = neutron::flat_hash_map<int, int, salted_hash, tagged_equal>;
    map source(16, salted_hash{ 0x5A5A }, tagged_equal{ 7 });
    fill(source, 0, 500);
    if (source.hash_function().salt != 0x5A5A || source.key_eq().tag != 7 ||
        !contains_all(source, 0, 500)) {
        return false;
    }
    map copy(source);
    map moved(std::move(copy));
    map assigned;
    assigned = moved;
    return assigned.hash_function().salt == 0x5A5A &&
           assigned.key_eq().tag == 7 && contains_all(assigned, 0, 500) &&
           moved.hash_function().salt == 0x5A5A;
}

bool test_set() {
    string_set set;
    if (!set.empty() || set.begin() != set.end()) {
        return false;
    }
    if (!set.insert("a").second || set.insert("a").second) {
        return false;
    }
    auto [iter, inserted] = set.emplace(3, 'x');
    if (!inserted || *iter != "xxx") {
        return false;
    }
    const std::string bee = "b";
    std::string cee       = "c";
    if (!set.emplace(bee).second || !set.emplace(cee).second ||
        !set.emplace(std::string("d")).second || set.emplace("d").second) {
        return false;
    }
    if (set.size() != 5 || set.count("xxx") != 1 ||
        set.find("zzz") != set.end()) {
        return false;
    }
    for (int i = 0; i != 1000; ++i) {
        set.insert(std::to_string(i));
    }
    if (set.size() != 1005 || distance_of(set) != 1005) {
        return false;
    }
    for (int i = 0; i < 1000; i += 2) {
        if (set.erase(std::to_string(i)) != 1) {
            return false;
        }
    }
    for (int i = 0; i != 1000; ++i) {
        if ((set.count(std::to_string(i)) == 1) != (i % 2 == 1)) {
            return false;
        }
    }

    string_set copy(set);
    if (copy != set || !(copy == set)) {
        return false;
    }
    copy.erase("a");
    if (copy == set) {
        return false;
    }
    string_set moved(std::move(copy));
    if (moved.size() != set.size() - 1 || !copy.empty()) {
        return false;
    }

    int_set numbers{ 1, 2, 3, 2, 1 };
    const int_set expected{ 3, 2, 1 };
    if (numbers.size() != 3 || numbers != expected) {
        return false;
    }
    int sum = 0;
    for (int val : std::as_const(numbers)) {
        sum += val;
    }
    numbers.erase(numbers.find(2));
    return sum == 6 && numbers.size() == 2 && numbers.count(2) == 0;
}

} // namespace

int main() {
    if (!test_default_state()) {
        return 1;
    }
    if (!test_insert_and_lookup()) {
        return 2;
    }
    if (!test_hint_and_assign()) {
        return 3;
    }
    if (!test_at_throws()) {
        return 4;
    }
    if (!test_growth()) {
        return 5;
    }
    if (!test_rehash_reserve_shrink()) {
        return 6;
    }
    if (!test_erase()) {
        return 7;
    }
    if (!test_range_erase()) {
        return 8;
    }
    if (!test_collisions()) {
        return 9;
    }
    if (!test_hash_policies()) {
        return 10;
    }
    if (!test_copy_and_move()) {
        return 11;
    }
    if (!test_equality()) {
        return 12;
    }
    if (!test_constructors()) {
        return 13;
    }
    if (!test_string_keys()) {
        return 14;
    }
    if (!test_object_lifetime()) {
        return 15;
    }
    if (!test_move_only()) {
        return 16;
    }
    if (!test_exception_safety()) {
        return 17;
    }
    if (!test_allocator()) {
        return 18;
    }
    if (!test_stateful_functors()) {
        return 19;
    }
    if (!test_set()) {
        return 20;
    }
    return 0;
}
