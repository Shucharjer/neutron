// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <type_traits>
#include "neutron/detail/concepts/allocator.hpp"

namespace neutron {

template <unfancy_allocator Alloc>
class archetype;

template <unfancy_allocator Alloc>
class world_base;

template <unfancy_allocator Alloc>
class command_buffer;

template <unfancy_allocator Alloc>
class basic_command_list;

template <unfancy_allocator Alloc>
class basic_commands;

template <typename Descriptor, unfancy_allocator Alloc>
class basic_world;

template <typename...>
class query;

namespace internal {

template <typename>
constexpr bool _is_world = false;
template <typename Descriptor, typename Alloc>
constexpr bool _is_world<basic_world<Descriptor, Alloc>> = true;

template <typename World>
concept world = _is_world<std::remove_cvref_t<World>>;

} // namespace internal
namespace _world_base {

template <unfancy_allocator>
class world_base;

}

} // namespace neutron
