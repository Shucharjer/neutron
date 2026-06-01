// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once

namespace neutron {

template <typename Alloc>
class archetype;

template <typename Alloc>
class world_base;

template <typename Alloc>
class command_buffer;

template <typename Alloc>
class basic_command_list;

template <typename Alloc>
class basic_commands;

template <typename Descriptor, typename Alloc>
class basic_world;

template <typename...>
class query;

namespace internal {

template <typename>
constexpr bool _is_world = false;
template <typename Descriptor, typename Alloc>
constexpr bool _is_world<basic_world<Descriptor, Alloc>> = true;

template <typename World>
concept world = _is_world<World>;

} // namespace internal
namespace _world_base {

template <typename>
class world_base;

}

} // namespace neutron
