// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#include <concepts>
#include <cstddef>
#include "neutron/detail/memory/rebind_alloc.hpp"

namespace neutron {

template <typename Alloc>
concept sized_allocator = requires(Alloc& alloc, std::size_t n) {
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

template <typename Alloc>
concept simple_allocator =
    std_simple_allocator<Alloc> ||
    std_simple_allocator<rebind_alloc_t<Alloc, std::byte>>;

} // namespace neutron
