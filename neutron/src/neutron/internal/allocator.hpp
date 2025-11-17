#pragma once
#include <concepts>
#include <cstddef>
#include <memory>

namespace neutron {

template <typename Alloc>
concept _sized_allocator = requires(Alloc& alloc, size_t n) {
    { *alloc.allocate(n) } -> std::same_as<typename Alloc::value_type&>;
    alloc.deallocate(alloc.allocate(n), n);
};

template <typename Alloc>
concept _allocator = requires(Alloc& alloc) {
    { alloc.allocate() } -> std::convertible_to<void*>;
};

// @note Concept `simple_allocator` since C++26.
template <typename Alloc>
concept _std_simple_allocator =
    _sized_allocator<Alloc> && std::copy_constructible<Alloc> &&
    std::equality_comparable<Alloc>;

template <typename Alloc, typename Ty>
using rebind_alloc_t =
    typename std::allocator_traits<Alloc>::template rebind_alloc<Ty>;

/*! @cond TURN_OFF_DOXYGEN */

template <typename Alloc>
constexpr auto _allocate(Alloc& alloc, size_t n = 1) {
    return std::allocator_traits<Alloc>::allocate(alloc, n);
}

template <typename Alloc, typename... Args>
constexpr void _construct(
    Alloc& alloc, typename std::allocator_traits<Alloc>::pointer ptr,
    Args&&... args) {
    std::uninitialized_construct_using_allocator(
        ptr, alloc, std::forward<Args>(args)...);
}

template <typename Alloc>
constexpr void _destroy(
    Alloc& alloc, typename std::allocator_traits<Alloc>::pointer
                      ptr) noexcept(std::
                                        is_nothrow_destructible_v<
                                            typename std::allocator_traits<
                                                Alloc>::value_type>) {
    return std::allocator_traits<Alloc>::destroy(alloc, ptr);
}

template <typename Alloc>
constexpr void _deallocate(
    Alloc& alloc, typename std::allocator_traits<Alloc>::pointer ptr,
    size_t n = 1) noexcept {
    return std::allocator_traits<Alloc>::deallocate(alloc, ptr, n);
}

/*! @endcond */

} // namespace neutron
