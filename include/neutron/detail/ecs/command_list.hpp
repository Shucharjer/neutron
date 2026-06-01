// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <cstddef>
#include <memory>
#include "neutron/detail/ecs/command_buffer.hpp"
#include "neutron/detail/ecs/component.hpp"

namespace neutron {

template <typename Alloc = std::allocator<std::byte>>
class basic_command_list {
public:
    constexpr basic_command_list(command_buffer<Alloc>& cb) noexcept
        : command_buffer_(cb) {}

    template <component... Components>
    requires(std::same_as<Components, std::remove_cvref_t<Components>> && ...)
    future_entity_t spawn() {
        return command_buffer_->template spawn<Components...>();
    }

    template <component... Components>
    future_entity_t spawn(Components&&... components) {
        return command_buffer_->spawn(std::forward<Components>(components)...);
    }

    template <component... Components>
    void add_components(future_entity_t entity) {
        return command_buffer_->template add_components<Components...>(entity);
    }

    template <component... Components>
    void add_components(entity_t entity) {
        return command_buffer_->template add_components<Components...>(entity);
    }

    template <typename Entity, component... Components>
    void add_components(Entity entity, Components&&... components) {
        return command_buffer_->add_components(
            entity, std::forward<Components>(components)...);
    }

    template <component... Components>
    void remove_components(future_entity_t entity) {
        return command_buffer_->template remove_components<Components...>(
            entity);
    }

    template <component... Components>
    void remove_components(entity_t entity) {
        return command_buffer_->template remove_components<Components...>(
            entity);
    }

    void kill(future_entity_t entity) { command_buffer_->kill(entity); }

    void kill(entity_t entity) { return command_buffer_->kill(entity); }

private:
    command_buffer<Alloc>* command_buffer_;
};

using command_list = basic_command_list<std::allocator<std::byte>>;

namespace pmr {

using command_list =
    basic_command_list<std::pmr::polymorphic_allocator<std::byte>>;

}

} // namespace neutron
