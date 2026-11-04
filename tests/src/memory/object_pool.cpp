#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <neutron/memory.hpp>

using namespace neutron;

namespace {

/// @brief Tracks live instances to catch leaks and double destroys.
struct counted {
    inline static int constructed = 0;
    inline static int destroyed   = 0;

    static void reset() noexcept {
        constructed = 0;
        destroyed   = 0;
    }

    static int alive() noexcept { return constructed - destroyed; }

    explicit counted(std::size_t value) : value(value), text(64, 'x') {
        ++constructed;
    }
    counted(const counted&)            = delete;
    counted& operator=(const counted&) = delete;
    ~counted() { ++destroyed; }

    std::size_t value;
    std::string text;
};

/// @brief Deletor accepting typed blocks; records calls.
template <std::size_t Size>
struct typed_deletor {
    int* calls;
    int* null_calls;

    void operator()(freeable_bytes<Size>* blocks, std::size_t) noexcept {
        if (blocks == nullptr) {
            ++*null_calls;
        } else {
            ++*calls;
            delete[] blocks;
        }
    }
};

/// @brief Deletor accepting void pointers; records released capacity.
template <std::size_t Size>
struct void_deletor {
    std::size_t* released;

    void operator()(void* blocks, std::size_t capacity) noexcept {
        *released += capacity;
        delete[] static_cast<freeable_bytes<Size>*>(blocks);
    }
};

/// @brief Stateless deletor, should not occupy storage.
template <std::size_t Size>
struct empty_deletor {
    void operator()(freeable_bytes<Size>* blocks, std::size_t) const noexcept {
        delete[] blocks;
    }
};

template <typename Deletor>
concept proxy_deletor = _pool_proxy<16, Deletor>::deletor_supports_typed ||
                        _pool_proxy<16, Deletor>::deletor_supports_void;

template <typename Pool, typename Ty>
concept takeable = requires(Pool& pool) { pool.template take<Ty>(); };

struct big {
    std::byte bytes[32]; // NOLINT(modernize-avoid-c-arrays)
};

static_assert(sizeof(freeable_bytes<16>) == 16);
static_assert(sizeof(freeable_bytes<4>) >= sizeof(void*));
static_assert(alignof(freeable_bytes<16>) == 8);
static_assert(std::is_trivially_default_constructible_v<freeable_bytes<16>>);

static_assert(!std::is_copy_constructible_v<_pool_proxy<16>>);
static_assert(!std::is_copy_assignable_v<_pool_proxy<16>>);
static_assert(std::is_nothrow_move_constructible_v<_pool_proxy<16>>);
static_assert(std::is_nothrow_move_assignable_v<_pool_proxy<16>>);
static_assert(std::is_nothrow_destructible_v<_pool_proxy<16>>);
static_assert(std::is_same_v<_pool_proxy<16>::deletor_type, void>);

static_assert(proxy_deletor<typed_deletor<16>>);
static_assert(proxy_deletor<void_deletor<16>>);
static_assert(_pool_proxy<16, typed_deletor<16>>::deletor_supports_typed);
static_assert(_pool_proxy<16, void_deletor<16>>::deletor_supports_void);
static_assert(
    !std::is_copy_constructible_v<_pool_proxy<16, typed_deletor<16>>>);
static_assert(
    std::is_nothrow_move_constructible_v<_pool_proxy<16, typed_deletor<16>>>);
static_assert(
    sizeof(_pool_proxy<16, empty_deletor<16>>) == sizeof(_pool_proxy<16>));

struct alignas(32) over_aligned {
    int value;
};

static_assert(takeable<_pool_proxy<16>, int>);
static_assert(takeable<_pool_proxy<16>, std::byte[16]>); // NOLINT
static_assert(!takeable<_pool_proxy<16>, big>);
static_assert(!takeable<_pool_proxy<32>, over_aligned>);
static_assert(takeable<_pool_proxy<32, void, 32>, over_aligned>);

// Pools reject objects that do not fit in, or are more aligned than, a block.
static_assert(!takeable<constcapacity_pool<8, 2>, std::string>);
static_assert(!takeable<runtime_pool<8>, std::string>);
static_assert(!takeable<constcapacity_pool<32, 2>, over_aligned>);
static_assert(takeable<constcapacity_pool<32, 2, 32>, over_aligned>);
static_assert(takeable<runtime_pool<32, 32>, over_aligned>);

// Every block is aligned, so the block size is a multiple of the alignment.
static_assert(sizeof(runtime_pool<16, 64>::block_type) == 64);
static_assert(sizeof(constcapacity_pool<4, 2>::block_type) == sizeof(void*));

static_assert(!std::is_copy_constructible_v<constcapacity_pool<16, 4>>);
static_assert(!std::is_move_constructible_v<constcapacity_pool<16, 4>>);
static_assert(!std::is_copy_assignable_v<constcapacity_pool<16, 4>>);
static_assert(!std::is_move_assignable_v<constcapacity_pool<16, 4>>);
static_assert(
    std::is_nothrow_default_constructible_v<constcapacity_pool<16, 4>>);
static_assert(!std::is_convertible_v<int, runtime_pool<16>>);
static_assert(std::is_constructible_v<runtime_pool<16>, std::size_t>);
static_assert(!std::is_copy_constructible_v<runtime_pool<16>>);
static_assert(!std::is_move_constructible_v<runtime_pool<16>>);

template <std::size_t Align>
concept valid_align = requires { typename constcapacity_pool<16, 4, Align>; };
static_assert(valid_align<8> && valid_align<16>);

using block16 = freeable_bytes<16>;

// Casting from `void*` in constant evaluation requires P2738 (C++26), and only
// the block type itself is usable there.
#if __cpp_constexpr >= 202306L

constexpr bool constcapacity_pool_in_constant_evaluation() {
    constcapacity_pool<16, 4> pool;
    block16* first  = pool.take<block16>();
    block16* second = pool.take<block16>();
    pool.put(first);
    return first != second && pool.take<block16>() == first;
}
static_assert(constcapacity_pool_in_constant_evaluation());

constexpr bool pool_proxy_in_constant_evaluation() {
    block16 blocks[3]; // NOLINT(modernize-avoid-c-arrays)
    _pool_proxy<16> proxy(blocks, 3);
    block16* a = proxy.take<block16>();
    block16* b = proxy.take<block16>();
    block16* c = proxy.take<block16>();
    block16* d = proxy.take<block16>();
    const bool order =
        a == &blocks[0] && b == &blocks[1] && c == &blocks[2] && d == nullptr;
    proxy.put(b);
    return order && proxy.take<block16>() == b && proxy.capacity() == 3;
}
static_assert(pool_proxy_in_constant_evaluation());
#endif

template <typename Pool>
bool take_all_distinct(Pool& pool, std::size_t capacity, std::size_t align) {
    std::vector<void*> taken;
    for (std::size_t i = 0; i != capacity; ++i) {
        void* pointer = pool.template take<std::byte>();
        if (pointer == nullptr ||
            reinterpret_cast<std::uintptr_t>(pointer) % align != 0) {
            return false;
        }
        taken.push_back(pointer);
    }
    if (pool.template take<std::byte>() != nullptr) {
        return false;
    }
    std::ranges::sort(taken);
    if (std::ranges::adjacent_find(taken) != taken.end()) {
        return false;
    }
    for (void* pointer : taken) {
        pool.put(pointer);
    }
    // Every block is reusable after being returned.
    std::vector<void*> again;
    for (std::size_t i = 0; i != capacity; ++i) {
        again.push_back(pool.template take<std::byte>());
    }
    std::ranges::sort(again);
    const bool same = again == taken;
    for (void* pointer : again) {
        pool.put(pointer);
    }
    return same;
}

bool test_constcapacity_take_put() {
    constexpr std::size_t capacity = 8;
    constcapacity_pool<sizeof(counted), capacity> pool;
    if (!take_all_distinct(pool, capacity, alignof(void*))) {
        return false;
    }

    // Returned blocks are reused in LIFO order.
    auto* first  = pool.take<counted>();
    auto* second = pool.take<counted>();
    pool.put(second);
    pool.put(first);
    if (pool.take<counted>() != first || pool.take<counted>() != second) {
        return false;
    }
    pool.put(first);
    pool.put(second);

    // Returning nullptr is a no-op.
    pool.put(nullptr);
    return take_all_distinct(pool, capacity, alignof(void*));
}

bool test_constcapacity_aligned() {
    constexpr std::size_t align = 64;
    constcapacity_pool<align, 4, align> pool;
    if (!take_all_distinct(pool, 4, align)) {
        return false;
    }
    // Blocks smaller than the alignment are still aligned.
    constcapacity_pool<16, 4, align> small;
    return take_all_distinct(small, 4, align);
}

bool test_constcapacity_single() {
    constcapacity_pool<sizeof(int), 1> pool;
    int* only = pool.take<int>();
    if (only == nullptr || pool.take<int>() != nullptr) {
        return false;
    }
    *only = 7;
    pool.put(only);
    return pool.take<int>() == only;
}

bool test_runtime_pool_counted() {
    constexpr std::size_t capacity = 1024;
    counted::reset();
    {
        runtime_pool<sizeof(counted)> pool(capacity);
        std::vector<counted*> objects;
        objects.reserve(capacity);
        for (std::size_t i = 0; i != capacity; ++i) {
            auto* block = pool.take<counted>();
            if (block == nullptr) {
                return false;
            }
            objects.push_back(std::construct_at(block, i));
        }
        if (pool.take<counted>() != nullptr || counted::alive() != capacity) {
            return false;
        }
        for (std::size_t i = 0; i != capacity; ++i) {
            if (objects[i]->value != i || objects[i]->text.size() != 64) {
                return false;
            }
        }

        // Release every other object and refill the holes.
        for (std::size_t i = 0; i < capacity; i += 2) {
            std::destroy_at(objects[i]);
            pool.put(objects[i]);
        }
        if (counted::alive() != capacity / 2) {
            return false;
        }
        for (std::size_t i = 0; i < capacity; i += 2) {
            auto* block = pool.take<counted>();
            if (block == nullptr) {
                return false;
            }
            objects[i] = std::construct_at(block, i + capacity);
        }
        if (pool.take<counted>() != nullptr) {
            return false;
        }
        for (std::size_t i = 0; i != capacity; ++i) {
            const auto expected = i % 2 == 0 ? i + capacity : i;
            if (objects[i]->value != expected) {
                return false;
            }
        }
        for (auto* object : objects) {
            std::destroy_at(object);
            pool.put(object);
        }
    }
    return counted::alive() == 0 && counted::constructed == capacity * 3 / 2;
}

bool test_runtime_pool_blocks() {
    runtime_pool<sizeof(std::string)> pool(64);
    if (!take_all_distinct(pool, 64, alignof(void*))) {
        return false;
    }
    runtime_pool<64, 64> aligned(16);
    if (!take_all_distinct(aligned, 16, 64)) {
        return false;
    }
    runtime_pool<16, 64> small_aligned(16);
    if (!take_all_distinct(small_aligned, 16, 64)) {
        return false;
    }
    // Blocks smaller than a pointer still hold the free list.
    runtime_pool<4> tiny(4);
    if (!take_all_distinct(tiny, 4, alignof(void*))) {
        return false;
    }
    runtime_pool<12> odd(5);
    return take_all_distinct(odd, 5, alignof(void*));
}

bool test_proxy_typed_blocks() {
    constexpr std::size_t capacity = 5;
    freeable_bytes<16> blocks[capacity]; // NOLINT(modernize-avoid-c-arrays)
    _pool_proxy<16> proxy(blocks, capacity);
    if (proxy.data() != blocks || proxy.capacity() != capacity ||
        std::as_const(proxy).data() != blocks) {
        return false;
    }
    for (std::size_t i = 0; i != capacity; ++i) {
        if (proxy.take<std::uint64_t>() !=
            static_cast<void*>(std::next(blocks, i))) {
            return false;
        }
    }
    return proxy.take<std::uint64_t>() == nullptr;
}

bool test_proxy_raw_blocks() {
    constexpr std::size_t capacity = 4;
    alignas(freeable_bytes<32>)
        std::byte raw[sizeof(freeable_bytes<32>) * capacity]; // NOLINT
    _pool_proxy<32> proxy(static_cast<void*>(raw), capacity);
    if (static_cast<void*>(proxy.data()) != raw) {
        return false;
    }
    return take_all_distinct(proxy, capacity, alignof(void*));
}

bool test_proxy_move() {
    freeable_bytes<16> lhs_blocks[2]; // NOLINT(modernize-avoid-c-arrays)
    freeable_bytes<16> rhs_blocks[3]; // NOLINT(modernize-avoid-c-arrays)
    _pool_proxy<16> lhs(lhs_blocks, 2);
    void* taken = lhs.take<int>();

    _pool_proxy<16> moved(std::move(lhs));
    // NOLINTBEGIN(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    if (lhs.data() != nullptr || lhs.capacity() != 0 ||
        lhs.take<int>() != nullptr) {
        return false;
    }
    // NOLINTEND(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    if (moved.data() != lhs_blocks || moved.capacity() != 2) {
        return false;
    }
    // The free list state moves along with the blocks.
    if (moved.take<block16>() != &lhs_blocks[1] ||
        moved.take<block16>() != nullptr) {
        return false;
    }
    moved.put(taken);

    _pool_proxy<16> rhs(rhs_blocks, 3);
    moved = std::move(rhs);
    if (moved.data() != rhs_blocks || moved.capacity() != 3) {
        return false;
    }
    // Self move assignment keeps the state.
    auto& self = moved;
    moved      = std::move(self);
    if (moved.data() != rhs_blocks || moved.capacity() != 3) {
        return false;
    }
    return moved.take<block16>() == &rhs_blocks[0];
}

bool test_proxy_typed_deletor() {
    int calls      = 0;
    int null_calls = 0;
    {
        using proxy_t = _pool_proxy<16, typed_deletor<16>>;
        proxy_t proxy(
            new freeable_bytes<16>[4], 4,
            typed_deletor<16>{ &calls, &null_calls });
        if (proxy.capacity() != 4 ||
            !take_all_distinct(proxy, 4, alignof(void*))) {
            return false;
        }
        proxy_t moved(std::move(proxy));
        if (moved.capacity() != 4) {
            return false;
        }

        proxy_t other(
            new freeable_bytes<16>[2], 2,
            typed_deletor<16>{ &calls, &null_calls });
        other = std::move(moved);
        if (other.capacity() != 4 || calls != 0) {
            return false;
        }
    }
    // Both allocations are released exactly once, the moved-from one passes
    // nullptr.
    return calls == 2 && null_calls == 1;
}

bool test_proxy_raw_blocks_with_deletor() {
    struct raw_deletor {
        int* calls;
        void operator()(void* blocks, std::size_t) noexcept {
            if (blocks != nullptr) {
                ++*calls;
                ::operator delete(blocks);
            }
        }
    };
    constexpr std::size_t capacity = 4;
    int calls                      = 0;
    {
        _pool_proxy<16, raw_deletor> proxy(
            ::operator new(sizeof(block16) * capacity), capacity,
            raw_deletor{ &calls });
        if (!take_all_distinct(proxy, capacity, alignof(void*))) {
            return false;
        }
    }
    return calls == 1;
}

bool test_proxy_void_deletor() {
    std::size_t released = 0;
    {
        _pool_proxy<16, void_deletor<16>> proxy(
            new freeable_bytes<16>[6], 6, void_deletor<16>{ &released });
        auto* value = proxy.take<std::uint32_t>();
        *value      = 42U;
        proxy.put(value);
    }
    return released == 6;
}

} // namespace

int main() {
    if (!test_constcapacity_take_put()) {
        return 1;
    }
    if (!test_constcapacity_aligned()) {
        return 2;
    }
    if (!test_constcapacity_single()) {
        return 3;
    }
    if (!test_runtime_pool_counted()) {
        return 4;
    }
    if (!test_runtime_pool_blocks()) {
        return 5;
    }
    if (!test_proxy_typed_blocks()) {
        return 6;
    }
    if (!test_proxy_raw_blocks()) {
        return 7;
    }
    if (!test_proxy_move()) {
        return 8;
    }
    if (!test_proxy_typed_deletor()) {
        return 9;
    }
    if (!test_proxy_void_deletor()) {
        return 10;
    }
    if (!test_proxy_raw_blocks_with_deletor()) {
        return 11;
    }
    return 0;
}
