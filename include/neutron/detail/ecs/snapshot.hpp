#pragma once
#include <cstddef>
#include <ranges>
#include <vector>
#include <neutron/ecs.hpp>
#include <neutron/memory.hpp>
#include "neutron/detail/ecs/archetype.hpp"
#include "neutron/detail/ecs/world_base.hpp"

namespace neutron {

template <typename Alloc>
class snapshot {
    template <typename T>
    using _allocator_t = rebind_alloc_t<Alloc, T>;

    template <typename T>
    using _vector_t = std::vector<T, _allocator_t<T>>;

public:
    template <typename Al = Alloc>
    snapshot(const Al& alloc = {}) : archetypes_(alloc) {}

    template <typename WorldAlloc, typename Al = Alloc>
    snapshot(const world_base<WorldAlloc>& world, const Al& alloc)
        : archetypes_(world.archetypes_ | std::views::values, alloc) {}

    template <typename Al = Alloc>
    snapshot(world_base<Al>& world) : snapshot(world, world.get_allocator()) {}

    template <typename Al = Alloc>
    void store(const world_base<Al>& world) {
        archetypes_ = world.archetypes_ | std::views::values;
    }

    template <typename Al = Alloc>
    void store(world_base<Al>&& world) {
        archetypes_ = std::move(world.archetypes_) | std::views::values;
    }

    template <typename Al = Alloc>
    void load(world_base<Al>& world) & {
        world.archetypes_.clear();
        for (const auto& archetype : archetypes_) {
            world.archetypes_.emplace(archetype.hash(), archetype);
        }
    }

    template <typename Al = Alloc>
    void load(world_base<Al>& world) && {
        world.archetypes_.clear();
        for (auto& archetype : archetypes_) {
            const auto hash = archetype.hash();
            world.archetypes_.emplace(hash, std::move(archetype));
        }
    }

private:
    _vector_t<archetype<_allocator_t<std::byte>>> archetypes_;
};

} // namespace neutron
