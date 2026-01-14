// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <cstddef>
#include <memory>
#include <neutron/detail/concepts/allocator.hpp>
#include <neutron/small_vector.hpp>

namespace neutron {

template <std_simple_allocator Alloc = std::allocator<std::byte>>
class archetype;

template <
    std_simple_allocator Alloc = std::allocator<std::byte>, size_t Count = 8,
    typename... Filters>
class basic_query;

template <std_simple_allocator Alloc = std::allocator<std::byte>>
class command_buffer;

template <std_simple_allocator Alloc = std::allocator<std::byte>>
class basic_commands;

template <
    typename Descriptor, std_simple_allocator Alloc = std::allocator<std::byte>>
class basic_world;

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template <typename>
constexpr bool _is_basic_world = false;
template <typename... Args>
constexpr bool _is_basic_world<basic_world<Args...>> = true;

} // namespace internal
/* @endcond */

template <typename Ty>
concept world = internal::_is_basic_world<Ty>;

} // namespace neutron
