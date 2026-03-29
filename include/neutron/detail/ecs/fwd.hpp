// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <cstddef>
#include <memory>
#include <neutron/detail/concepts/allocator.hpp>

namespace neutron {

template <std_simple_allocator Alloc = std::allocator<std::byte>>
class archetype;

template <typename... Filters>
class query;

template <std_simple_allocator Alloc = std::allocator<std::byte>>
class command_buffer;

template <
    std_simple_allocator Alloc = std::allocator<std::byte>, bool Direct = false>
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

class insertion_context;

namespace _world_base {

template <std_simple_allocator Alloc = std::allocator<std::byte>>
class world_base;

} // namespace _world_base

} // namespace neutron
