// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <concepts>
#include <memory_resource>
#include <utility>
#include "neutron/detail/ecs/command_buffer.hpp"
#include "neutron/detail/ecs/construct_from_world.hpp"

namespace neutron {

template <std_simple_allocator Alloc>
class basic_commands {
public:
    explicit constexpr basic_commands(
        command_buffer<Alloc>& command_buffer) noexcept
        : command_buffer_(&command_buffer) {}

    future_entity_t spawn() { return command_buffer_->spawn(); }

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

    command_buffer<Alloc>* get_command_buffer() const noexcept {
        return command_buffer_;
    }

private:
    command_buffer<Alloc>* command_buffer_;
};

template <auto Sys, std_simple_allocator Alloc, size_t Index>
struct construct_from_world_t<Sys, basic_commands<Alloc>, Index> {
    template <world World>
    basic_commands<Alloc> operator()(World& world) const noexcept {
        const auto cmdbuf_index = Index % world.command_buffers_->size();
        return basic_commands<Alloc>{ world.command_buffers_->at(
            cmdbuf_index) };
    }
};

namespace pmr {

using commands = basic_commands<std::pmr::polymorphic_allocator<>>;

} // namespace pmr

} // namespace neutron
