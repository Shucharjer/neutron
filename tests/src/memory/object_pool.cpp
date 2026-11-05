#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <neutron/memory.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

using block16 = freeable_bytes<16>;

/// Tracks constructions and destructions to catch leaks and double destroys.
struct counted {
    static constexpr std::size_t text_length = 64;

    inline static int constructed = 0;
    inline static int destroyed   = 0;

    static void reset() noexcept {
        constructed = 0;
        destroyed   = 0;
    }

    static int alive() noexcept { return constructed - destroyed; }

    explicit counted(std::size_t value) : value(value), text(text_length, 'x') {
        ++constructed;
    }
    counted(const counted&)            = delete;
    counted& operator=(const counted&) = delete;
    ~counted() { ++destroyed; }

    std::size_t value;
    std::string text;
};

/// Deletor accepting typed blocks; counts calls with and without blocks.
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

/// Deletor accepting void pointers; sums the released capacity.
template <std::size_t Size>
struct void_deletor {
    std::size_t* released;

    void operator()(void* blocks, std::size_t capacity) noexcept {
        *released += capacity;
        delete[] static_cast<freeable_bytes<Size>*>(blocks);
    }
};

/// Stateless deletor, should not occupy storage.
template <std::size_t Size>
struct empty_deletor {
    void operator()(freeable_bytes<Size>* blocks, std::size_t) const noexcept {
        delete[] blocks;
    }
};

/// Takes `capacity` blocks, checks they are distinct and aligned and that the
/// pool is then exhausted, returns them and checks the same blocks come back.
/// Leaves every block in the pool. Returns false if any check failed.
template <typename Pool>
bool takes_all_distinct(Pool& pool, std::size_t capacity, std::size_t align) {
    bool passed = true;

    std::vector<void*> taken;
    for (std::size_t i = 0; i != capacity; ++i) {
        void* const pointer = pool.template take<std::byte>();
        passed &= require(pointer != nullptr);
        passed &=
            require(reinterpret_cast<std::uintptr_t>(pointer) % align == 0);
        taken.push_back(pointer);
    }
    passed &= require(pool.template take<std::byte>() == nullptr);

    std::ranges::sort(taken);
    passed &= require(std::ranges::adjacent_find(taken) == taken.end());

    for (void* const pointer : taken) {
        pool.put(pointer);
    }

    // Every block is reusable after being returned.
    std::vector<void*> again;
    for (std::size_t i = 0; i != capacity; ++i) {
        again.push_back(pool.template take<std::byte>());
    }
    std::ranges::sort(again);
    passed &= require(again == taken);

    for (void* const pointer : again) {
        pool.put(pointer);
    }
    return passed;
}

template <typename Deletor>
concept proxy_deletor = _pool_proxy<16, Deletor>::deletor_supports_typed ||
                        _pool_proxy<16, Deletor>::deletor_supports_void;

template <typename Pool, typename Ty>
concept takeable = requires(Pool& pool) { pool.template take<Ty>(); };

struct big {
    std::byte bytes[32]; // NOLINT(modernize-avoid-c-arrays)
};

struct alignas(32) over_aligned {
    int value;
};

// Block layout: at least a pointer, aligned for the free list.
static_assert(sizeof(freeable_bytes<16>) == 16);
static_assert(sizeof(freeable_bytes<4>) >= sizeof(void*));
static_assert(alignof(freeable_bytes<16>) == 8);
static_assert(std::is_trivially_default_constructible_v<freeable_bytes<16>>);

// A proxy without deletor is a move-only view over borrowed blocks.
static_assert(!std::is_copy_constructible_v<_pool_proxy<16>>);
static_assert(!std::is_copy_assignable_v<_pool_proxy<16>>);
static_assert(std::is_nothrow_move_constructible_v<_pool_proxy<16>>);
static_assert(std::is_nothrow_move_assignable_v<_pool_proxy<16>>);
static_assert(std::is_nothrow_destructible_v<_pool_proxy<16>>);
static_assert(std::is_same_v<_pool_proxy<16>::deletor_type, void>);

// Deletors may take typed or void blocks; stateless ones cost no space.
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

// A proxy rejects objects that do not fit in, or are more aligned than, a
// block.
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

// Pools own their blocks, so they are neither copyable nor movable.
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

// Pointer alignment and above are accepted.
static_assert(valid_align<8>);
static_assert(valid_align<16>);

// Casting from `void*` in constant evaluation requires P2738 (C++26), and only
// the block type itself is usable there.
#if __cpp_constexpr >= 202306L

/// True if a constcapacity_pool hands out distinct blocks and reuses a
/// returned one in constant evaluation.
constexpr bool constcapacity_pool_in_constant_evaluation() {
    constcapacity_pool<16, 4> pool;
    block16* first  = pool.take<block16>();
    block16* second = pool.take<block16>();

    pool.put(first);

    return first != second && pool.take<block16>() == first;
}
static_assert(constcapacity_pool_in_constant_evaluation());

/// True if a proxy takes blocks in order, is then exhausted and reuses a
/// returned block in constant evaluation.
constexpr bool pool_proxy_in_constant_evaluation() {
    constexpr std::size_t capacity = 3;
    block16 blocks[capacity]; // NOLINT(modernize-avoid-c-arrays)
    _pool_proxy<16> proxy(blocks, capacity);
    block16* a = proxy.take<block16>();
    block16* b = proxy.take<block16>();
    block16* c = proxy.take<block16>();
    block16* d = proxy.take<block16>();
    const bool order =
        a == &blocks[0] && b == &blocks[1] && c == &blocks[2] && d == nullptr;

    proxy.put(b);

    return order && proxy.take<block16>() == b && proxy.capacity() == capacity;
}
static_assert(pool_proxy_in_constant_evaluation());

#endif

void test_constcapacity_pool_blocks_are_distinct_and_reusable() {
    constexpr std::size_t capacity = 8;
    constcapacity_pool<sizeof(counted), capacity> pool;

    require(takes_all_distinct(pool, capacity, alignof(void*)));
}

void test_constcapacity_pool_reuses_blocks_lifo() {
    constexpr std::size_t capacity = 8;
    constcapacity_pool<sizeof(counted), capacity> pool;
    auto* const first  = pool.take<counted>();
    auto* const second = pool.take<counted>();

    pool.put(second);
    pool.put(first);

    require(pool.take<counted>() == first);
    require(pool.take<counted>() == second);

    pool.put(first);
    pool.put(second);

    require(takes_all_distinct(pool, capacity, alignof(void*)));
}

void test_constcapacity_pool_put_null_is_noop() {
    constexpr std::size_t capacity = 8;
    constcapacity_pool<sizeof(counted), capacity> pool;

    pool.put(nullptr);

    // A null block would show up as an extra or null take.
    require(takes_all_distinct(pool, capacity, alignof(void*)));
}

void test_constcapacity_pool_blocks_are_aligned() {
    constexpr std::size_t align    = 64;
    constexpr std::size_t capacity = 4;
    constcapacity_pool<align, capacity, align> pool;
    // Blocks smaller than the alignment are still aligned.
    constcapacity_pool<16, capacity, align> small;

    require(takes_all_distinct(pool, capacity, align));
    require(takes_all_distinct(small, capacity, align));
}

void test_single_block_pool_hands_out_one_block() {
    constexpr int written = 7;
    constcapacity_pool<sizeof(int), 1> pool;

    int* const only = pool.take<int>();

    require_or_return(only != nullptr, );
    require(pool.take<int>() == nullptr);

    *only = written;
    pool.put(only);

    require(pool.take<int>() == only);
}

void test_runtime_pool_reuses_released_blocks_for_objects() {
    constexpr std::size_t capacity = 1024;
    counted::reset();
    {
        runtime_pool<sizeof(counted)> pool(capacity);
        std::vector<counted*> objects;
        objects.reserve(capacity);

        for (std::size_t i = 0; i != capacity; ++i) {
            auto* const block = pool.take<counted>();
            require_or_return(block != nullptr, );
            objects.push_back(std::construct_at(block, i));
        }

        require(pool.take<counted>() == nullptr);
        require(counted::alive() == capacity);
        for (std::size_t i = 0; i != capacity; ++i) {
            require(objects[i]->value == i);
            require(objects[i]->text.size() == counted::text_length);
        }

        // Release every other object and refill the holes.
        for (std::size_t i = 0; i < capacity; i += 2) {
            std::destroy_at(objects[i]);
            pool.put(objects[i]);
        }

        require(counted::alive() == capacity / 2);

        for (std::size_t i = 0; i < capacity; i += 2) {
            auto* const block = pool.take<counted>();
            require_or_return(block != nullptr, );
            objects[i] = std::construct_at(block, i + capacity);
        }

        require(pool.take<counted>() == nullptr);
        for (std::size_t i = 0; i != capacity; ++i) {
            const auto expected = i % 2 == 0 ? i + capacity : i;
            require(objects[i]->value == expected);
        }

        for (auto* const object : objects) {
            std::destroy_at(object);
            pool.put(object);
        }
    }

    require(counted::alive() == 0);
    require(counted::constructed == capacity * 3 / 2);
}

void test_runtime_pool_blocks_are_distinct_and_reusable() {
    constexpr std::size_t capacity     = 64;
    constexpr std::size_t odd_capacity = 5;
    runtime_pool<sizeof(std::string)> pool(capacity);
    runtime_pool<12> odd(odd_capacity);

    require(takes_all_distinct(pool, capacity, alignof(void*)));
    require(takes_all_distinct(odd, odd_capacity, alignof(void*)));
}

void test_runtime_pool_blocks_are_aligned() {
    constexpr std::size_t align    = 64;
    constexpr std::size_t capacity = 16;
    runtime_pool<64, align> aligned(capacity);
    // Blocks smaller than the alignment are still aligned.
    runtime_pool<16, align> small_aligned(capacity);

    require(takes_all_distinct(aligned, capacity, align));
    require(takes_all_distinct(small_aligned, capacity, align));
}

void test_runtime_pool_blocks_smaller_than_pointer_hold_free_list() {
    constexpr std::size_t capacity = 4;
    runtime_pool<4> tiny(capacity);

    require(takes_all_distinct(tiny, capacity, alignof(void*)));
}

void test_proxy_exposes_blocks_and_capacity() {
    constexpr std::size_t capacity = 5;
    freeable_bytes<16> blocks[capacity]; // NOLINT(modernize-avoid-c-arrays)

    _pool_proxy<16> proxy(blocks, capacity);

    require(proxy.data() == blocks);
    require(std::as_const(proxy).data() == blocks);
    require(proxy.capacity() == capacity);
}

void test_proxy_takes_blocks_in_address_order() {
    constexpr std::size_t capacity = 5;
    freeable_bytes<16> blocks[capacity]; // NOLINT(modernize-avoid-c-arrays)
    _pool_proxy<16> proxy(blocks, capacity);

    for (std::size_t i = 0; i != capacity; ++i) {
        void* const expected = std::next(blocks, i);
        require(proxy.take<std::uint64_t>() == expected);
    }
    require(proxy.take<std::uint64_t>() == nullptr);
}

void test_proxy_over_raw_bytes_hands_out_blocks() {
    constexpr std::size_t capacity = 4;
    alignas(freeable_bytes<32>)
        std::byte raw[sizeof(freeable_bytes<32>) * capacity]; // NOLINT

    _pool_proxy<32> proxy(static_cast<void*>(raw), capacity);

    require(static_cast<void*>(proxy.data()) == raw);
    require(takes_all_distinct(proxy, capacity, alignof(void*)));
}

void test_proxy_move_construction_transfers_free_list() {
    constexpr std::size_t capacity = 2;
    freeable_bytes<16> blocks[capacity]; // NOLINT(modernize-avoid-c-arrays)
    _pool_proxy<16> source(blocks, capacity);
    void* const taken = source.take<int>();

    _pool_proxy<16> moved(std::move(source));

    // NOLINTBEGIN(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    require(source.data() == nullptr);
    require(source.capacity() == 0);
    require(source.take<int>() == nullptr);
    // NOLINTEND(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    require(moved.data() == blocks);
    require(moved.capacity() == capacity);
    // The free list state moves along with the blocks.
    require(moved.take<block16>() == &blocks[1]);
    require(moved.take<block16>() == nullptr);

    moved.put(taken);
}

void test_proxy_move_assignment_adopts_blocks() {
    constexpr std::size_t lhs_capacity = 2;
    constexpr std::size_t rhs_capacity = 3;
    freeable_bytes<16>
        lhs_blocks[lhs_capacity]; // NOLINT(modernize-avoid-c-arrays)
    freeable_bytes<16>
        rhs_blocks[rhs_capacity]; // NOLINT(modernize-avoid-c-arrays)
    _pool_proxy<16> target(lhs_blocks, lhs_capacity);
    _pool_proxy<16> source(rhs_blocks, rhs_capacity);

    target = std::move(source);

    require(target.data() == rhs_blocks);
    require(target.capacity() == rhs_capacity);
    require(target.take<block16>() == &rhs_blocks[0]);
}

void test_proxy_self_move_assignment_keeps_state() {
    constexpr std::size_t capacity = 3;
    freeable_bytes<16> blocks[capacity]; // NOLINT(modernize-avoid-c-arrays)
    _pool_proxy<16> proxy(blocks, capacity);
    auto& self = proxy;

    proxy = std::move(self);

    require(proxy.data() == blocks);
    require(proxy.capacity() == capacity);
    require(proxy.take<block16>() == &blocks[0]);
}

void test_typed_deletor_releases_each_allocation_once() {
    using proxy_t                        = _pool_proxy<16, typed_deletor<16>>;
    constexpr std::size_t capacity       = 4;
    constexpr std::size_t other_capacity = 2;
    int calls                            = 0;
    int null_calls                       = 0;
    {
        proxy_t proxy(
            new freeable_bytes<16>[capacity], capacity, typed_deletor<16> {
                &calls, &null_calls
            });
        require(proxy.capacity() == capacity);
        require(takes_all_distinct(proxy, capacity, alignof(void*)));

        proxy_t moved(std::move(proxy));
        require(moved.capacity() == capacity);

        proxy_t other(
            new freeable_bytes<16>[other_capacity], other_capacity,
            typed_deletor<16> {
                &calls, &null_calls
            });
        other = std::move(moved);
        require(other.capacity() == capacity);
        require(calls == 0);
    }

    // Move assignment swaps, so `moved` releases the blocks of `other`; only
    // the moved-from `proxy` passes nullptr.
    require(calls == 2);
    require(null_calls == 1);
}

void test_void_deletor_receives_capacity() {
    constexpr std::size_t capacity  = 6;
    constexpr std::uint32_t written = 42;
    std::size_t released            = 0;
    {
        _pool_proxy<16, void_deletor<16>> proxy(
            new freeable_bytes<16>[capacity], capacity, void_deletor<16> {
                &released
            });
        auto* const value = proxy.take<std::uint32_t>();
        require_or_return(value != nullptr, );
        *value = written;
        proxy.put(value);
    }

    require(released == capacity);
}

void test_raw_deletor_releases_raw_allocation_once() {
    /// Frees `operator new` memory; counts non-null calls.
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
        require(takes_all_distinct(proxy, capacity, alignof(void*)));
    }

    require(calls == 1);
}

} // namespace

int main() {
    test_constcapacity_pool_blocks_are_distinct_and_reusable();
    test_constcapacity_pool_reuses_blocks_lifo();
    test_constcapacity_pool_put_null_is_noop();
    test_constcapacity_pool_blocks_are_aligned();
    test_single_block_pool_hands_out_one_block();
    test_runtime_pool_reuses_released_blocks_for_objects();
    test_runtime_pool_blocks_are_distinct_and_reusable();
    test_runtime_pool_blocks_are_aligned();
    test_runtime_pool_blocks_smaller_than_pointer_hold_free_list();
    test_proxy_exposes_blocks_and_capacity();
    test_proxy_takes_blocks_in_address_order();
    test_proxy_over_raw_bytes_hands_out_blocks();
    test_proxy_move_construction_transfers_free_list();
    test_proxy_move_assignment_adopts_blocks();
    test_proxy_self_move_assignment_keeps_state();
    test_typed_deletor_releases_each_allocation_once();
    test_void_deletor_receives_capacity();
    test_raw_deletor_releases_raw_allocation_once();
    return neutron_test::result();
}
