// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <type_traits>
#endif
#include "neutron/detail/concepts/allocator.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <unfancy_allocator Alloc>
class archetype;

ATOM_EXPORT template <unfancy_allocator Alloc>
class world_base;

ATOM_EXPORT template <unfancy_allocator Alloc>
class command_buffer;

ATOM_EXPORT template <unfancy_allocator Alloc>
class basic_command_list;

ATOM_EXPORT template <unfancy_allocator Alloc>
class basic_commands;

ATOM_EXPORT template <typename Descriptor, unfancy_allocator Alloc>
class basic_world;

ATOM_EXPORT template <typename...>
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
