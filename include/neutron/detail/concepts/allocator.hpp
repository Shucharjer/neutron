// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>
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

template <typename Alloc>
concept unfancy_allocator =
    simple_allocator<Alloc> &&
    std::is_pointer_v<typename std::allocator_traits<Alloc>::pointer>;

template <typename Alloc>
concept unfancy_byte_allocator =
    unfancy_allocator<Alloc> &&
    std::same_as<typename std::allocator_traits<Alloc>::value_type, std::byte>;

} // namespace neutron
