#pragma once
#include <memory>

namespace neutron {

template <typename Alloc>
constexpr auto traits_allocate(Alloc& alloc, size_t n = 1) {
    return std::allocator_traits<Alloc>::allocate(alloc, n);
}

template <typename Alloc, typename... Args>
constexpr void traits_construct(
    Alloc& alloc, typename std::allocator_traits<Alloc>::pointer ptr,
    Args&&... args) {
    std::uninitialized_construct_using_allocator(
        ptr, alloc, std::forward<Args>(args)...);
}

template <typename Alloc>
constexpr void traits_destroy(
    Alloc& alloc, typename std::allocator_traits<Alloc>::pointer
                      ptr) noexcept(std::
                                        is_nothrow_destructible_v<
                                            typename std::allocator_traits<
                                                Alloc>::value_type>) {
    return std::allocator_traits<Alloc>::destroy(alloc, ptr);
}

template <typename Alloc>
constexpr void traits_deallocate(
    Alloc& alloc, typename std::allocator_traits<Alloc>::pointer ptr,
    size_t n = 1) noexcept {
    return std::allocator_traits<Alloc>::deallocate(alloc, ptr, n);
}

} // namespace neutron
