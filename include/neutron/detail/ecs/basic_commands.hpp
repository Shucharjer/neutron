// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <concepts>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <utility>
#include <neutron/concepts.hpp>
#include <neutron/ecs.hpp>
#include "neutron/detail/ecs/command_buffer.hpp"
#include "neutron/detail/ecs/construct_from_world.hpp"
#include "neutron/detail/ecs/fwd.hpp"

namespace neutron {

template <std_simple_allocator Alloc>
class basic_commands<Alloc, false> {
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

private:
    command_buffer<Alloc>* command_buffer_;
};

template <std_simple_allocator Alloc>
class basic_commands<Alloc, true> {
public:
    template <world World>
    explicit basic_commands(World& world) noexcept : world_(&world) {}

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

template <std_simple_allocator Alloc>
basic_commands(command_buffer<Alloc>&) -> basic_commands<Alloc, false>;

template <typename Descriptor, std_simple_allocator Alloc>
basic_commands(basic_world<Descriptor, Alloc>&) -> basic_commands<Alloc, true>;

template <auto Sys, std_simple_allocator Alloc, size_t Index>
struct construct_from_world_t<Sys, basic_commands<Alloc, false>, Index> {
    template <world World>
    basic_commands<Alloc, false> operator()(World& world) const noexcept {
        const auto cmdbuf_index = Index % world.command_buffers_.size();
        return basic_commands<Alloc>{ world.command_buffers_[cmdbuf_index] };
    }
};

template <auto Sys, std_simple_allocator Alloc, size_t Index>
struct construct_from_world_t<Sys, basic_commands<Alloc, true>, Index> {
    template <world World>
    basic_commands<Alloc, true> operator()(World& world) const noexcept {
        return basic_commands<Alloc, false>{ world };
    }
};

using commands        = basic_commands<std::allocator<std::byte>>;
using direct_commands = basic_commands<std::allocator<std::byte>, true>;

namespace pmr {

using commands        = basic_commands<std::pmr::polymorphic_allocator<>>;
using direct_commands = basic_commands<std::pmr::polymorphic_allocator<>, true>;

} // namespace pmr

} // namespace neutron
