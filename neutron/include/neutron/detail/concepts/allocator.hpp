#pragma once
#include <concepts>

namespace neutron {

template <typename Alloc>
concept sized_allocator = requires(Alloc& alloc, size_t n) {
    { *alloc.allocate(n) } -> std::same_as<typename Alloc::value_type&>;
    alloc.deallocate(alloc.allocate(n), n);
};

template <typename Alloc>
concept allocator = requires(Alloc& alloc) {
    { *alloc.allocate() } -> std::same_as<typename Alloc::value_type&>;
    alloc.deallocate(alloc.allocate());
};

// @note Concept `simple_allocator` since C++26.
template <typename Alloc>
concept std_simple_allocator =
    sized_allocator<Alloc> && std::copy_constructible<Alloc> &&
    std::equality_comparable<Alloc>;

} // namespace neutron
