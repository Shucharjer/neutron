#include <cstddef>
#include <neutron/ecs.hpp>
#include "neutron/detail/ecs/world_accessor.hpp"
#include "require.hpp"

using namespace neutron;

auto entity_archetype(auto& world, entity_t entity) {
    return world_accessor::entities(world)[static_cast<size_t>(entity)].second;
}

struct Position {
    using component_concept = neutron::component_t;
    float x{ 0 };
    float y{ 0 };
};

struct Velocity {
    using component_concept = neutron::component_t;
    float x{ 0 };
    float y{ 0 };
};

struct Tag {
    using component_concept = neutron::component_t;
};

void require_position_only(
    auto* archetype, float x, float y, size_t expected_count = 1) {
    require(archetype != nullptr);

    size_t count = 0;
    for (auto [position] : view_of<Position&>(*archetype)) {
        require(position.x == x && position.y == y);
        ++count;
    }
    require(count == expected_count);
}

void require_position_velocity(
    auto* archetype, float px, float py, float vx, float vy,
    size_t expected_count = 1) {
    require(archetype != nullptr);

    size_t count = 0;
    for (auto [position, velocity] :
         view_of<Position&, Velocity&>(*archetype)) {
        require(position.x == px && position.y == py);
        require(velocity.x == vx && velocity.y == vy);
        ++count;
    }
    require(count == expected_count);
}

void require_entity_has_no_archetype(auto& world, entity_t entity) {
    const auto index = static_cast<size_t>(entity);
    auto& entities   = world_accessor::entities(world);
    require(index < entities.size());
    require(entities[index].first == entity);
    require(entities[index].second == nullptr);
}

void test_add_components_with_values() {
    basic_world<world_descriptor_t<>> world;
    const auto entity = world.spawn(Position{ 1, 2 }, Tag{});

    world.add_components(entity, Velocity{ 3, 4 });

    auto* archetype = entity_archetype(world, entity);
    require(archetype->template has<Position, Velocity, Tag>());
    require_position_velocity(archetype, 1, 2, 3, 4);
}

void test_remove_components() {
    basic_world<world_descriptor_t<>> world;
    const auto entity = world.spawn(Position{ 1, 2 }, Velocity{ 3, 4 }, Tag{});

    world.remove_components<Tag>(entity);

    auto* archetype = entity_archetype(world, entity);
    require(archetype->template has<Position>());
    require(archetype->template has<Velocity>());
    require_false(archetype->template has<Tag>());
    require_position_velocity(archetype, 1, 2, 3, 4);

    world.remove_components<Position>(entity);

    archetype = entity_archetype(world, entity);
    require_false(archetype->template has<Position>());
    require(archetype->template has<Velocity>());
    size_t count = 0;
    for (auto [velocity] : view_of<Velocity&>(*archetype)) {
        require(velocity.x == 3 && velocity.y == 4);
        ++count;
    }
    require(count == 1);

    world.remove_components<Velocity>(entity);
    require_entity_has_no_archetype(world, entity);
}

void test_add_components_without_values() {
    basic_world<world_descriptor_t<>> world;
    const auto entity = world.spawn(Position{ 2, 3 });

    world.add_components<Velocity, Tag>(entity);

    auto* archetype = entity_archetype(world, entity);
    require(archetype->template has<Position, Velocity, Tag>());
    require_position_velocity(archetype, 2, 3, 0, 0);

    world.remove_components<Velocity, Tag>(entity);

    archetype = entity_archetype(world, entity);
    require(archetype->template has<Position>());
    require_false(archetype->template has<Velocity>());
    require_false(archetype->template has<Tag>());
    require_position_only(archetype, 2, 3);
}

void test_add_components_overlap_is_noop() {
    basic_world<world_descriptor_t<>> world;
    const auto entity = world.spawn(Position{ 5, 6 });

    world.add_components(entity, Position{ 9, 9 }, Velocity{ 7, 8 });
    world.add_components<Position, Tag>(entity);

    auto* archetype = entity_archetype(world, entity);
    require(archetype->template has<Position, Velocity, Tag>());
    require_position_velocity(archetype, 5, 6, 7, 8);
}

void test_remove_missing_components_is_noop() {
    basic_world<world_descriptor_t<>> world;
    const auto entity = world.spawn(Position{ 5, 6 }, Velocity{ 7, 8 }, Tag{});

    world.remove_components<Tag>(entity);
    world.remove_components<Tag>(entity);

    auto* archetype = entity_archetype(world, entity);
    require(archetype->template has<Position, Velocity>());
    require_false(archetype->template has<Tag>());
    require_position_velocity(archetype, 5, 6, 7, 8);
}

void test_command_buffer_future_add_components() {
    basic_world<world_descriptor_t<>> world;
    command_buffer<> buffer;

    const auto future = buffer.spawn(Position{ 10, 11 });
    buffer.add_components(future, Velocity{ 12, 13 });
    buffer.add_components<Tag>(future);
    buffer.apply(world);

    auto& entities        = world_accessor::entities(world);
    auto* const archetype = entities[1].second;
    require(archetype->template has<Position, Velocity, Tag>());
    require_position_velocity(archetype, 10, 11, 12, 13);
}

void test_command_buffer_future_remove_components() {
    basic_world<world_descriptor_t<>> world;
    command_buffer<> buffer;

    const auto future = buffer.spawn(Position{ 20, 21 }, Velocity{ 22, 23 });
    buffer.remove_components<Velocity>(future);
    buffer.apply(world);

    auto& entities        = world_accessor::entities(world);
    auto* const archetype = entities[1].second;
    require(archetype->template has<Position>());
    require_false(archetype->template has<Velocity>());
    require_position_only(archetype, 20, 21);
}

int main() {
    test_add_components_with_values();
    neutron::println("world_base test: add values ok");
    test_remove_components();
    neutron::println("world_base test: remove ok");
    test_add_components_without_values();
    neutron::println("world_base test: add default ok");
    test_add_components_overlap_is_noop();
    neutron::println("world_base test: add overlap ok");
    test_remove_missing_components_is_noop();
    neutron::println("world_base test: remove noop ok");
    test_command_buffer_future_add_components();
    neutron::println("world_base test: command buffer add ok");
    test_command_buffer_future_remove_components();
    neutron::println("world_base test: command buffer remove ok");
    return 0;
}
