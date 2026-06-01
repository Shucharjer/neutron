// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <cstddef>
#include <memory>
#include <memory_resource>
#include "neutron/detail/ecs/component.hpp"
#include "neutron/detail/ecs/entity.hpp"

namespace neutron {

template <typename Alloc = std::allocator<std::byte>>
class basic_commands {
public:
    constexpr basic_commands(world_base<Alloc>& world) noexcept
        : world_(&world) {}

    entity_t spawn() { return world_->spawn(); }

    template <component... Components>
    requires(std::same_as<Components, std::remove_cvref_t<Components>> && ...)
    entity_t spawn() {
        return world_->template spawn<Components...>();
    }

    template <component... Components>
    entity_t spawn(Components&&... components) {
        return world_->spawn(std::forward<Components>(components)...);
    }

    template <component... Components>
    void add_components(entity_t entity) {
        return world_->template add_components<Components...>(entity);
    }

    template <component... Components>
    void add_components(entity_t entity, Components&&... components) {
        return world_->add_components(
            entity, std::forward<Components>(components)...);
    }

    template <component... Components>
    void remove_components(entity_t entity) {
        return world_->template remove_components<Components...>(entity);
    }

    void kill(entity_t entity) { return world_->kill(entity); }

private:
    world_base<Alloc>* world_;
};

using commands = basic_commands<std::allocator<std::byte>>;

namespace pmr {

using commands = basic_commands<std::pmr::polymorphic_allocator<std::byte>>;

}

} // namespace neutron
