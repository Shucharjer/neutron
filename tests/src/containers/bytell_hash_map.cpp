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

namespace {

using neutron::bytell_hash_map;
using neutron::bytell_hash_set;

using int_map    = bytell_hash_map<int, int>;
using string_map = bytell_hash_map<std::string, std::string>;
using int_set    = bytell_hash_set<int>;

static_assert(std::is_same_v<int_map::key_type, int>);
static_assert(std::is_same_v<int_map::mapped_type, int>);
static_assert(std::is_same_v<int_map::value_type, std::pair<int, int>>);
static_assert(std::is_same_v<int_map::size_type, std::size_t>);
static_assert(std::is_same_v<int_set::key_type, int>);
static_assert(std::is_same_v<int_set::value_type, int>);

static_assert(std::forward_iterator<int_map::iterator>);
static_assert(std::forward_iterator<int_map::const_iterator>);
static_assert(std::forward_iterator<int_set::iterator>);
static_assert(std::forward_iterator<int_set::const_iterator>);
static_assert(std::ranges::forward_range<string_map>);
static_assert(std::ranges::forward_range<const string_map>);
static_assert(std::ranges::common_range<string_map>);
static_assert(std::ranges::forward_range<int_set>);
static_assert(std::is_same_v<
              std::iter_reference_t<int_map::const_iterator>,
              const std::pair<int, int>&>);
static_assert(
    std::is_convertible_v<int_map::iterator, int_map::const_iterator>);
static_assert(
    !std::is_convertible_v<int_map::const_iterator, int_map::iterator>);

static_assert(std::is_copy_constructible_v<string_map>);
static_assert(std::is_nothrow_move_constructible_v<string_map>);
static_assert(std::is_copy_assignable_v<string_map>);
static_assert(std::is_move_assignable_v<string_map>);

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

struct counted_hash {
    std::size_t operator()(const counted& c) const {
        return std::hash<int>{}(c.value);
    }
};

void reset_counted() {
    counted::live          = 0;
    counted::bad_destroy   = 0;
    counted::throw_on_copy = -1;
}

bool counted_clean() { return counted::live == 0 && counted::bad_destroy == 0; }

/// Bookkeeping shared by `tracking_allocator` copies.
struct arena {
    std::map<void*, std::size_t> blocks;
    int bad_deallocations = 0;
};

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

using tracked_map = bytell_hash_map<
    int, std::string, std::hash<int>, std::equal_to<int>,
    tracking_allocator<std::pair<int, std::string>>>;

// Moving with a possibly unequal allocator may allocate.
static_assert(!std::is_nothrow_constructible_v<
              tracked_map, tracked_map&&,
              const tracking_allocator<std::pair<int, std::string>>&>);
static_assert(!std::is_nothrow_move_assignable_v<tracked_map>);
static_assert(std::is_nothrow_move_assignable_v<bytell_hash_map<
                  int, std::string, std::hash<int>, std::equal_to<int>,
                  tracking_allocator<std::pair<int, std::string>, true>>>);
static_assert(std::is_nothrow_constructible_v<
              bytell_hash_map<int, int>, bytell_hash_map<int, int>&&,
              const std::allocator<std::pair<int, int>>&>);

bool arena_clean(const arena& a) {
    return a.blocks.empty() && a.bad_deallocations == 0;
}

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

template <typename Map>
void fill_doubled(Map& map, int first, int last, int step = 1) {
    for (int key = first; key < last; key += step) {
        map.emplace(key, key * 2);
    }
}

bool test_default_state() {
    int_map map;
    const int_map& cmap = map;
    if (!map.empty() || map.size() != 0 || map.bucket_count() != 0 ||
        map.load_factor() != 0.0F || map.max_size() == 0) {
        return false;
    }
    if (map.begin() != map.end() || cmap.begin() != cmap.end() ||
        map.cbegin() != map.cend()) {
        return false;
    }
    if (map.find(1) != map.end() || cmap.find(1) != cmap.end() ||
        map.count(1) != 0 || map.erase(1) != 0) {
        return false;
    }
    map.clear();
    map.shrink_to_fit();
    return map.empty() && map.begin() == map.end();
}

bool test_insert_and_lookup() {
    int_map map;

    auto [iter, inserted] = map.insert({ 1, 10 });
    if (!inserted || iter->first != 1 || iter->second != 10 ||
        map.size() != 1 || map.empty()) {
        return false;
    }

    // Duplicate keys keep the original value.
    auto dup = map.emplace(1, 99);
    if (dup.second || dup.first != iter || dup.first->second != 10) {
        return false;
    }

    std::pair<int, int> value{ 2, 20 };
    if (!map.insert(value).second || map.insert(value).second) {
        return false;
    }
    if (map.emplace_hint(map.begin(), 3, 30)->second != 30) {
        return false;
    }
    if (map.insert(map.cbegin(), std::pair{ 4, 40 })->second != 40) {
        return false;
    }

    map[5] = 50;
    ++map[5];
    if (map[5] != 51 || map[6] != 0 || map.size() != 6) {
        return false;
    }

    // Default key and value.
    if (!map.emplace().second || map.at(0) != 0) {
        return false;
    }

    auto range = map.equal_range(2);
    if (std::distance(range.first, range.second) != 1 ||
        range.first->second != 20) {
        return false;
    }
    auto missing = map.equal_range(100);
    if (missing.first != map.end() || missing.second != map.end()) {
        return false;
    }

    const int_map& cmap = map;
    auto crange         = cmap.equal_range(3);
    if (crange.first == cmap.end() || crange.first->second != 30 ||
        std::next(crange.first) != crange.second) {
        return false;
    }

    return map.count(4) == 1 && map.count(100) == 0 && cmap.at(4) == 40 &&
           map.bucket(4) < map.bucket_count();
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

    const int_map& cmap = map;
    thrown              = false;
    ATOM_TRY { static_cast<void>(cmap.at(2)); }
    ATOM_CATCH(const std::out_of_range&) { thrown = true; }
    return thrown && map.size() == 1;
}

bool test_insert_or_assign() {
    string_map map;
    std::string key = "key";

    auto first = map.insert_or_assign(key, "a");
    if (!first.second || first.first->second != "a") {
        return false;
    }
    auto second = map.insert_or_assign(std::string("key"), std::string("b"));
    if (second.second || second.first->second != "b" || map.size() != 1) {
        return false;
    }

    auto hinted = map.insert_or_assign(map.cbegin(), key, "c");
    if (hinted->second != "c") {
        return false;
    }
    hinted = map.insert_or_assign(map.cbegin(), std::string("other"), "d");
    return hinted->first == "other" && hinted->second == "d" && map.size() == 2;
}

bool test_constructors() {
    const std::vector<std::pair<int, int>> source{
        { 1, 2 },
        { 2, 4 },
        { 3, 6 },
        { 1, 8 }
    };

    int_map from_range(source.begin(), source.end());
    if (!contains_exactly(from_range, 1, 4)) {
        return false;
    }

    int_map from_list{
        { 1, 2 },
        { 2, 4 },
        { 3, 6 }
    };
    if (!contains_exactly(from_list, 1, 4) || from_list != from_range) {
        return false;
    }

    int_map with_buckets(100);
    if (with_buckets.bucket_count() < 100 || !with_buckets.empty()) {
        return false;
    }

    int_map with_list_buckets(
        {
            { 1, 2 }
    },
        64);
    if (with_list_buckets.bucket_count() < 64 ||
        !contains_exactly(with_list_buckets, 1, 2)) {
        return false;
    }

    int_set set{ 1, 2, 3, 2 };
    return set.size() == 3 && set.count(2) == 1;
}

bool test_growth() {
    constexpr int count = 10000;

    int_map map;
    std::size_t last_buckets = map.bucket_count();
    int rehashes             = 0;
    for (int key = 0; key < count; ++key) {
        if (!map.emplace(key, key * 2).second) {
            return false;
        }
        if (map.bucket_count() != last_buckets) {
            last_buckets = map.bucket_count();
            ++rehashes;
        }
        if (map.load_factor() > map.max_load_factor()) {
            return false;
        }
    }
    if (rehashes < 2 || !contains_exactly(map, 0, count)) {
        return false;
    }
    // The default policy uses power of two bucket counts.
    if ((map.bucket_count() & (map.bucket_count() - 1)) != 0) {
        return false;
    }

    // Each element is visited exactly once.
    std::vector<int> seen(count, 0);
    std::size_t visited = 0;
    for (const auto& [key, value] : map) {
        if (key < 0 || key >= count || value != key * 2) {
            return false;
        }
        ++seen[static_cast<std::size_t>(key)];
        ++visited;
    }
    if (visited != map.size()) {
        return false;
    }
    for (int times : seen) {
        if (times != 1) {
            return false;
        }
    }

    // Values are mutable through iterators.
    for (auto& [key, value] : map) {
        value = key * 3;
    }
    return map.at(1234) == 1234 * 3;
}

bool test_erase() {
    constexpr int count = 2000;

    int_map map;
    fill_doubled(map, 0, count);

    // Erase by key.
    for (int key = 0; key < count; key += 2) {
        if (map.erase(key) != 1) {
            return false;
        }
    }
    if (map.erase(0) != 0 || !contains_exactly(map, 1, count, 2)) {
        return false;
    }

    // Reinsert erased keys.
    fill_doubled(map, 0, count, 2);
    if (!contains_exactly(map, 0, count)) {
        return false;
    }

    // Erase by iterator.
    for (int key = 0; key < count; key += 3) {
        auto iter = map.find(key);
        if (iter == map.end()) {
            return false;
        }
        map.erase(iter);
    }
    for (int key = 0; key < count; ++key) {
        if (map.count(key) != (key % 3 == 0 ? 0U : 1U)) {
            return false;
        }
    }

    // Erasing the only element yields end().
    int_map single{
        { 7, 14 }
    };
    int_map::iterator next = single.erase(single.begin());
    if (next != single.end() || !single.empty()) {
        return false;
    }
    return single.emplace(7, 14).second && single.size() == 1;
}

bool test_erase_while_iterating() {
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
    return contains_exactly(map, 1, count, 2);
}

bool test_erase_range() {
    constexpr int count = 300;

    int_map all;
    fill_doubled(all, 0, count);
    all.erase(all.cbegin(), all.cend());
    if (!all.empty() || all.begin() != all.end()) {
        return false;
    }

    for (int offset = 0; offset < 50; ++offset) {
        int_map map;
        for (int key = 0; key < count; ++key) {
            map.emplace(key * 13 + offset, key);
        }
        auto first = std::next(map.begin(), 50);
        auto last  = std::next(first, 100);
        std::set<int> erased;
        for (auto iter = first; iter != last; ++iter) {
            erased.insert(iter->first);
        }

        auto empty_range = map.erase(first, first);
        if (empty_range != first) {
            return false;
        }
        map.erase(first, last);

        if (map.size() != count - erased.size()) {
            return false;
        }
        for (int key = 0; key < count; ++key) {
            const int actual = key * 13 + offset;
            if ((map.count(actual) == 1) == erased.contains(actual)) {
                return false;
            }
        }
    }
    return true;
}

bool test_string_elements() {
    // Long strings force heap allocations so ASan catches leaks.
    auto make = [](int index) {
        return std::string(40, 'x') + std::to_string(index);
    };

    string_map map;
    for (int i = 0; i < 1000; ++i) {
        map[make(i)] = make(i * 2);
    }
    for (int i = 0; i < 1000; i += 2) {
        map.erase(make(i));
    }
    for (int i = 0; i < 1000; ++i) {
        auto iter = map.find(make(i));
        if ((iter == map.end()) != (i % 2 == 0)) {
            return false;
        }
        if (iter != map.end() && iter->second != make(i * 2)) {
            return false;
        }
    }

    // Lookup with convertible keys.
    map.emplace("short", "value");
    return map.at("short") == "value" && map.size() == 501;
}

bool test_copy_and_move() {
    string_map source;
    for (int i = 0; i < 100; ++i) {
        source.emplace(std::to_string(i), std::string(30, 'v'));
    }

    string_map copy(source);
    if (copy != source || copy.size() != 100) {
        return false;
    }
    copy["0"] = "changed";
    if (copy == source || source.at("0") != std::string(30, 'v')) {
        return false;
    }

    string_map assigned;
    assigned["stale"] = "stale";
    assigned          = source;
    if (assigned != source || assigned.count("stale") != 0) {
        return false;
    }

    // Self assignment.
    auto& alias = assigned;
    assigned    = alias;
    if (assigned != source) {
        return false;
    }
    assigned = std::move(alias);
    if (assigned != source) {
        return false;
    }

    string_map moved(std::move(copy));
    // NOLINTBEGIN(bugprone-use-after-move)
    if (moved.size() != 100 || !copy.empty() || copy.begin() != copy.end()) {
        return false;
    }
    copy["reuse"] = "ok";
    if (copy.size() != 1 || copy.at("reuse") != "ok") {
        return false;
    }

    string_map move_assigned;
    move_assigned["stale"] = "stale";
    move_assigned          = std::move(moved);
    if (move_assigned.size() != 100 || move_assigned.count("stale") != 0 ||
        move_assigned.at("0") != "changed") {
        return false;
    }
    // NOLINTEND(bugprone-use-after-move)

    // Copy of an empty map.
    string_map empty;
    string_map empty_copy(empty);
    return empty_copy.empty() && empty_copy == empty;
}

bool test_swap() {
    int_map lhs;
    int_map rhs;
    fill_doubled(lhs, 0, 100);
    fill_doubled(rhs, 1000, 1010);
    lhs.max_load_factor(0.5F);

    lhs.swap(rhs);
    if (!contains_exactly(lhs, 1000, 1010) || !contains_exactly(rhs, 0, 100) ||
        rhs.max_load_factor() != 0.5F) {
        return false;
    }

    using std::swap;
    swap(lhs, rhs);
    return contains_exactly(lhs, 0, 100) && contains_exactly(rhs, 1000, 1010);
}

bool test_equality() {
    int_map lhs;
    int_map rhs;
    fill_doubled(lhs, 0, 50);
    // Different insertion order and bucket count.
    rhs.reserve(1000);
    for (int key = 49; key >= 0; --key) {
        rhs.emplace(key, key * 2);
    }
    if (lhs != rhs || !(lhs == rhs)) {
        return false;
    }
    rhs[0] = 1;
    if (lhs == rhs) {
        return false;
    }
    rhs.erase(0);
    return lhs != rhs;
}

bool test_reserve_rehash() {
    constexpr int count = 1000;

    int_map map;
    map.reserve(count);
    const std::size_t reserved = map.bucket_count();
    if (reserved < count) {
        return false;
    }
    fill_doubled(map, 0, count);
    if (map.bucket_count() != reserved) {
        return false;
    }

    // Smaller requests never drop elements.
    map.reserve(1);
    map.rehash(1);
    if (!contains_exactly(map, 0, count) ||
        map.load_factor() > map.max_load_factor()) {
        return false;
    }

    map.rehash(reserved * 4);
    if (map.bucket_count() < reserved * 4 || !contains_exactly(map, 0, count)) {
        return false;
    }

    for (int key = 10; key < count; ++key) {
        map.erase(key);
    }
    map.shrink_to_fit();
    if (map.bucket_count() >= reserved || !contains_exactly(map, 0, 10)) {
        return false;
    }

    map.clear();
    map.shrink_to_fit();
    if (map.bucket_count() != 0 || !map.empty()) {
        return false;
    }
    map.rehash(0);
    fill_doubled(map, 0, 10);
    if (!contains_exactly(map, 0, 10)) {
        return false;
    }

    // A lower load factor leads to more buckets.
    int_map sparse;
    sparse.max_load_factor(0.25F);
    fill_doubled(sparse, 0, count);
    return sparse.max_load_factor() == 0.25F && sparse.load_factor() <= 0.25F &&
           contains_exactly(sparse, 0, count);
}

struct prime_hash : std::hash<int> {
    using hash_policy = neutron::prime_number_hash_policy;
};

template <typename Hash>
bool check_policy() {
    constexpr int count = 5000;

    bytell_hash_map<int, int, Hash> map;
    fill_doubled(map, 0, count);
    if (!contains_exactly(map, 0, count)) {
        return false;
    }
    for (int key = 0; key < count; key += 2) {
        map.erase(key);
    }
    if (!contains_exactly(map, 1, count, 2)) {
        return false;
    }
    fill_doubled(map, 0, count, 2);
    map.rehash(count * 3);
    return contains_exactly(map, 0, count);
}

bool test_hash_policies() {
    return check_policy<neutron::power_of_two_std_hash<int>>() &&
           check_policy<prime_hash>();
}

struct constant_hash {
    std::size_t operator()(int) const noexcept { return 42; }
};

struct coarse_hash {
    std::size_t operator()(int key) const noexcept {
        return static_cast<std::size_t>(key % 3);
    }
};

template <typename Hash>
bool check_collisions(int count) {
    bytell_hash_map<int, int, Hash> map;
    fill_doubled(map, 0, count);
    if (!contains_exactly(map, 0, count)) {
        return false;
    }
    // Erasing chain heads and middles relinks the chain.
    for (int key = 0; key < count; key += 2) {
        if (map.erase(key) != 1) {
            return false;
        }
    }
    if (!contains_exactly(map, 1, count, 2)) {
        return false;
    }
    fill_doubled(map, 0, count, 2);
    return contains_exactly(map, 0, count);
}

bool test_collisions() {
    return check_collisions<constant_hash>(200) &&
           check_collisions<coarse_hash>(300);
}

bool test_counted_lifetime() {
    reset_counted();
    {
        bytell_hash_set<counted, counted_hash> set;
        for (int i = 0; i < 1000; ++i) {
            set.emplace(i);
        }
        if (counted::live != 1000) {
            return false;
        }
        for (int i = 0; i < 1000; i += 3) {
            set.erase(counted{ i });
        }
        auto copy = set;
        set.clear();
        if (!set.empty() || counted::live != static_cast<int>(copy.size())) {
            return false;
        }
        auto moved = std::move(copy);
        set        = moved;
        set.rehash(4096);
        set.shrink_to_fit();
        moved = std::move(set);
    }
    if (!counted_clean()) {
        return false;
    }

    {
        bytell_hash_map<int, counted> map;
        for (int i = 0; i < 500; ++i) {
            map[i].value = i;
        }
        map.erase(map.begin(), std::next(map.begin(), 100));
        if (counted::live != 400 || map.size() != 400) {
            return false;
        }
    }
    return counted_clean();
}

bool test_exception_safety() {
    reset_counted();
    {
        bytell_hash_set<counted, counted_hash> source;
        for (int i = 0; i < 100; ++i) {
            source.emplace(i);
        }

        // Copy construction rolls back on failure.
        counted::throw_on_copy = 50;
        bool thrown            = false;
        ATOM_TRY {
            bytell_hash_set<counted, counted_hash> copy(source);
            static_cast<void>(copy);
        }
        ATOM_CATCH(const std::runtime_error&) { thrown = true; }
        counted::throw_on_copy = -1;
        if (!thrown || counted::live != 100 || source.size() != 100) {
            return false;
        }

        // A throwing element constructor leaves the table untouched.
        const counted value{ 1000 };
        counted::throw_on_copy = 0;
        thrown                 = false;
        ATOM_TRY { source.insert(value); }
        ATOM_CATCH(const std::runtime_error&) { thrown = true; }
        counted::throw_on_copy = -1;
        if (!thrown || source.size() != 100 || source.count(value) != 0 ||
            counted::live != 101) {
            return false;
        }
        for (int i = 0; i < 100; ++i) {
            if (source.count(counted{ i }) != 1) {
                return false;
            }
        }
    }
    return counted_clean();
}

bool test_move_only_values() {
    bytell_hash_map<int, std::unique_ptr<int>> map;
    for (int i = 0; i < 500; ++i) {
        map.emplace(i, std::make_unique<int>(i));
    }
    map[1000] = std::make_unique<int>(1000);
    map.rehash(4096);
    for (int i = 0; i < 500; ++i) {
        if (*map.at(i) != i) {
            return false;
        }
    }
    auto moved = std::move(map);
    return moved.size() == 501 && *moved.at(1000) == 1000;
}

bool test_aligned_elements() {
    bytell_hash_map<long long, double> map;
    for (int i = 0; i < 1000; ++i) {
        map.emplace(i, i * 0.5);
    }
    for (const auto& [key, value] : map) {
        const auto address = reinterpret_cast<std::uintptr_t>(&key);
        if (address % alignof(std::pair<long long, double>) != 0 ||
            value != static_cast<double>(key) * 0.5) {
            return false;
        }
    }
    if (map.size() != 1000) {
        return false;
    }

    // Over-aligned elements, including lookups in the shared empty block.
    struct alignas(64) big {
        int value;
        bool operator==(const big&) const = default;
    };
    struct big_hash {
        std::size_t operator()(const big& item) const noexcept {
            return static_cast<std::size_t>(item.value);
        }
    };
    bytell_hash_set<big, big_hash> set;
    if (set.find(big{ 1 }) != set.end()) {
        return false;
    }
    for (int i = 0; i < 256; ++i) {
        set.insert(big{ i });
    }
    for (const big& item : set) {
        if (reinterpret_cast<std::uintptr_t>(&item) % alignof(big) != 0) {
            return false;
        }
    }
    return set.size() == 256;
}

bool test_allocator() {
    using pair_type  = std::pair<int, std::string>;
    using alloc_type = tracking_allocator<pair_type>;
    using map_type   = bytell_hash_map<
          int, std::string, std::hash<int>, std::equal_to<int>, alloc_type>;

    arena first;
    arena second;
    {
        map_type map{ alloc_type{ &first } };
        for (int i = 0; i < 200; ++i) {
            map.emplace(i, std::string(30, 'a'));
        }
        if (map.get_allocator().owner != &first || first.blocks.size() != 1) {
            return false;
        }

        // Copy keeps the allocator; allocator-extended copy uses the new one.
        map_type copy(map);
        map_type other(map, alloc_type{ &second });
        if (copy.get_allocator().owner != &first ||
            other.get_allocator().owner != &second || copy != map ||
            other != map || second.blocks.size() != 1) {
            return false;
        }

        // Allocator-extended move: element-wise with an unequal allocator,
        // stealing with an equal one.
        map_type source(map, alloc_type{ &first });
        map_type moved_other(std::move(source), alloc_type{ &second });
        if (moved_other.get_allocator().owner != &second ||
            moved_other != map || !source.empty()) {
            return false;
        }
        const std::size_t second_blocks = second.blocks.size();
        map_type stolen_other(std::move(moved_other), alloc_type{ &second });
        if (stolen_other != map || second.blocks.size() != second_blocks) {
            return false;
        }

        // Non-propagating, unequal allocators: element-wise move.
        other = std::move(copy);
        if (other.get_allocator().owner != &second || other != map) {
            return false;
        }

        // Non-propagating copy assignment keeps the target allocator.
        map_type target{ alloc_type{ &second } };
        target = map;
        if (target.get_allocator().owner != &second || target != map) {
            return false;
        }

        // Equal allocators steal the storage.
        map_type stolen{ alloc_type{ &first } };
        const std::size_t blocks = first.blocks.size();
        stolen                   = std::move(map);
        if (stolen.size() != 200 || first.blocks.size() > blocks) {
            return false;
        }
    }
    return arena_clean(first) && arena_clean(second);
}

bool test_propagating_allocator() {
    using alloc_type = tracking_allocator<int, true>;
    using set_type =
        bytell_hash_set<int, std::hash<int>, std::equal_to<int>, alloc_type>;

    arena first;
    arena second;
    {
        set_type lhs{ alloc_type{ &first } };
        set_type rhs{ alloc_type{ &second } };
        for (int i = 0; i < 100; ++i) {
            lhs.insert(i);
            rhs.insert(i + 1000);
        }

        set_type copied{ alloc_type{ &second } };
        copied.insert(-1);
        copied = lhs;
        if (copied.get_allocator().owner != &first || copied != lhs) {
            return false;
        }

        set_type moved{ alloc_type{ &second } };
        moved.insert(-1);
        moved = std::move(rhs);
        if (moved.get_allocator().owner != &second || moved.size() != 100 ||
            moved.count(1000) != 1) {
            return false;
        }

        lhs.swap(moved);
        if (lhs.get_allocator().owner != &second ||
            moved.get_allocator().owner != &first || lhs.count(1000) != 1 ||
            moved.count(0) != 1) {
            return false;
        }
    }
    return arena_clean(first) && arena_clean(second);
}

bool test_set() {
    bytell_hash_set<std::string> set;
    if (!set.emplace(3, 'a').second || !set.emplace("bbb").second) {
        return false;
    }
    const std::string ccc = "ccc";
    std::string ddd       = "ddd";
    if (!set.emplace(ccc).second || !set.emplace(ddd).second ||
        !set.emplace(std::string("eee")).second || set.emplace("aaa").second) {
        return false;
    }
    if (set.size() != 5 || set.count("aaa") != 1 || set.count("zzz") != 0) {
        return false;
    }

    bytell_hash_set<std::string> other{ "eee", "ddd", "ccc", "bbb", "aaa" };
    if (set != other) {
        return false;
    }
    other.erase("aaa");
    if (set == other) {
        return false;
    }

    std::set<std::string> visited(set.begin(), set.end());
    if (visited.size() != set.size()) {
        return false;
    }

    int_set numbers;
    for (int i = 0; i < 3000; ++i) {
        numbers.insert(i);
    }
    for (int i = 0; i < 3000; i += 2) {
        numbers.erase(i);
    }
    for (int i = 0; i < 3000; ++i) {
        if (numbers.count(i) != static_cast<std::size_t>(i % 2)) {
            return false;
        }
    }
    return numbers.size() == 1500;
}

} // namespace

int main() {
    if (!test_default_state()) {
        return 1;
    }
    if (!test_insert_and_lookup()) {
        return 2;
    }
    if (!test_at_throws()) {
        return 3;
    }
    if (!test_insert_or_assign()) {
        return 4;
    }
    if (!test_constructors()) {
        return 5;
    }
    if (!test_growth()) {
        return 6;
    }
    if (!test_erase()) {
        return 7;
    }
    if (!test_erase_while_iterating()) {
        return 8;
    }
    if (!test_erase_range()) {
        return 9;
    }
    if (!test_string_elements()) {
        return 10;
    }
    if (!test_copy_and_move()) {
        return 11;
    }
    if (!test_swap()) {
        return 12;
    }
    if (!test_equality()) {
        return 13;
    }
    if (!test_reserve_rehash()) {
        return 14;
    }
    if (!test_hash_policies()) {
        return 15;
    }
    if (!test_collisions()) {
        return 16;
    }
    if (!test_counted_lifetime()) {
        return 17;
    }
    if (!test_exception_safety()) {
        return 18;
    }
    if (!test_move_only_values()) {
        return 19;
    }
    if (!test_aligned_elements()) {
        return 20;
    }
    if (!test_allocator()) {
        return 21;
    }
    if (!test_propagating_allocator()) {
        return 22;
    }
    if (!test_set()) {
        return 23;
    }
    return 0;
}
