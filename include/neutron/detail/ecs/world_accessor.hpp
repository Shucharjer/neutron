// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron {

struct world_accessor {
    template <world World>
    ATOM_NODISCARD static decltype(auto) base(World& world) noexcept {
        return world._base();
    }
    template <world World>
    static auto& archetypes(World& world) noexcept {
        return world.archetypes_;
    }
    template <world World>
    static auto& entities(World& world) noexcept {
        return world.entities_;
    }
    template <world World>
    static auto& locals(World& world) noexcept {
        return world.locals_;
    }
    template <world World>
    static auto& resources(World& world) noexcept {
        return world.resources_;
    }
    template <world World>
    static auto& queries(World& world) noexcept {
        return world.queries_;
    }
    template <world World>
    static auto version(World& world) noexcept {
        return world.archetypes_.size();
    }
    template <world World>
    static auto& insertion_context(World& world) noexcept {
        return world.insertion_context_;
    }
};

} // namespace neutron
