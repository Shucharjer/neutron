#include <cstddef>
#include <memory>
#include <memory_resource>
#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Sized allocator that is copyable and equality comparable.
template <typename T>
struct sized_alloc {
    using value_type = T;
    sized_alloc()    = default;
    template <typename U>
    constexpr sized_alloc(const sized_alloc<U>&) noexcept {}
    T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
    void deallocate(T* ptr, std::size_t n) noexcept {
        std::allocator<T>{}.deallocate(ptr, n);
    }
    friend bool operator==(const sized_alloc&, const sized_alloc&) = default;
};

/// Sized allocator without equality comparison.
template <typename T>
struct incomparable_alloc {
    using value_type = T;
    T* allocate(std::size_t);
    void deallocate(T*, std::size_t) noexcept;
};

/// Sized allocator that cannot be copied.
template <typename T>
struct move_only_alloc {
    using value_type                        = T;
    move_only_alloc()                       = default;
    move_only_alloc(move_only_alloc&&)      = default;
    move_only_alloc(const move_only_alloc&) = delete;
    T* allocate(std::size_t);
    void deallocate(T*, std::size_t) noexcept;
    friend bool
        operator==(const move_only_alloc&, const move_only_alloc&) = default;
};

/// Not comparable, so only its `std::byte` rebinding (the specialization
/// below) is a standard simple allocator.
template <typename T>
struct byte_only_alloc {
    using value_type = T;
    template <typename U>
    struct rebind {
        using other = byte_only_alloc<U>;
    };
    T* allocate(std::size_t);
    void deallocate(T*, std::size_t) noexcept;
};

/// The comparable `std::byte` rebinding of `byte_only_alloc`.
template <>
struct byte_only_alloc<std::byte> {
    using value_type = std::byte;
    template <typename U>
    struct rebind {
        using other = byte_only_alloc<U>;
    };
    std::byte* allocate(std::size_t);
    void deallocate(std::byte*, std::size_t) noexcept;
    friend bool
        operator==(const byte_only_alloc&, const byte_only_alloc&) = default;
};

/// Single object allocator.
template <typename T>
struct object_alloc {
    using value_type = T;
    T* allocate();
    void deallocate(T*) noexcept;
};

/// Returns a pointer to the wrong type.
struct mismatched_alloc {
    using value_type = int;
    long* allocate(std::size_t);
    void deallocate(long*, std::size_t) noexcept;
    long* allocate();
    void deallocate(long*) noexcept;
};

/// Missing deallocate.
struct leaking_alloc {
    using value_type = int;
    int* allocate(std::size_t);
    int* allocate();
};

/// Missing value_type.
struct untyped_alloc {
    int* allocate(std::size_t);
    void deallocate(int*, std::size_t) noexcept;
};

// sized_allocator: satisfied by
static_assert(sized_allocator<std::allocator<int>>);
static_assert(sized_allocator<std::pmr::polymorphic_allocator<int>>);
static_assert(sized_allocator<sized_alloc<int>>);
static_assert(sized_allocator<incomparable_alloc<int>>);
static_assert(sized_allocator<move_only_alloc<int>>);

// sized_allocator: rejected
static_assert(!sized_allocator<object_alloc<int>>);
static_assert(!sized_allocator<mismatched_alloc>);
static_assert(!sized_allocator<leaking_alloc>);
static_assert(!sized_allocator<untyped_alloc>);
static_assert(!sized_allocator<int>);
static_assert(!sized_allocator<void>);

// sized_allocator: rejected: references and cv-qualified types
static_assert(!sized_allocator<const std::allocator<int>>);
static_assert(!sized_allocator<std::allocator<int>&>);

// allocator (single object allocation): satisfied by
static_assert(allocator<object_alloc<int>>);

// allocator: rejected
static_assert(!allocator<std::allocator<int>>);
static_assert(!allocator<sized_alloc<int>>);
static_assert(!allocator<mismatched_alloc>);
static_assert(!allocator<leaking_alloc>);
static_assert(!allocator<int>);

// allocator: rejected: references and cv-qualified types
static_assert(!allocator<const object_alloc<int>>);
static_assert(!allocator<object_alloc<int>&>);

// std_simple_allocator: satisfied by
static_assert(std_simple_allocator<std::allocator<int>>);
static_assert(std_simple_allocator<std::allocator<std::byte>>);
static_assert(std_simple_allocator<std::pmr::polymorphic_allocator<int>>);
static_assert(std_simple_allocator<sized_alloc<int>>);
static_assert(std_simple_allocator<byte_only_alloc<std::byte>>);

// std_simple_allocator: rejected
static_assert(!std_simple_allocator<incomparable_alloc<int>>);
static_assert(!std_simple_allocator<move_only_alloc<int>>);
static_assert(!std_simple_allocator<byte_only_alloc<int>>);
static_assert(!std_simple_allocator<object_alloc<int>>);
static_assert(!std_simple_allocator<const std::allocator<int>>);

// simple_allocator: satisfied by, also when only the byte rebinding qualifies
static_assert(simple_allocator<std::allocator<int>>);
static_assert(simple_allocator<sized_alloc<int>>);
static_assert(simple_allocator<byte_only_alloc<int>>);
static_assert(simple_allocator<byte_only_alloc<std::byte>>);

// simple_allocator: rejected
static_assert(!simple_allocator<incomparable_alloc<int>>);
static_assert(!simple_allocator<move_only_alloc<int>>);
static_assert(!simple_allocator<object_alloc<int>>);

void test_stateless_allocators_compare_equal() {
    require(sized_alloc<int>{} == sized_alloc<int>{});
    require(move_only_alloc<int>{} == move_only_alloc<int>{});
    require(byte_only_alloc<std::byte>{} == byte_only_alloc<std::byte>{});
}

void test_rebound_copy_compares_equal() {
    const sized_alloc<int> alloc;

    const sized_alloc<std::byte> rebound(alloc);

    require(rebound == sized_alloc<std::byte>{});
}

void test_allocated_storage_is_writable() {
    constexpr std::size_t count = 4;
    constexpr int first_value   = 1;
    constexpr int last_value    = 4;
    sized_alloc<int> alloc;

    int* const ptr = alloc.allocate(count);
    require_or_return(ptr != nullptr, );
    ptr[0]         = first_value;
    ptr[count - 1] = last_value;

    require(ptr[0] == first_value);
    require(ptr[count - 1] == last_value);
    alloc.deallocate(ptr, count);
}

} // namespace

int main() {
    test_stateless_allocators_compare_equal();
    test_rebound_copy_compares_equal();
    test_allocated_storage_is_writable();
    return neutron_test::result();
}
