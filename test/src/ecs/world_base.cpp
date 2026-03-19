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

void test_add_and_remove_components() {
    basic_world<world_descriptor_t<>> world;
    const auto entity = world.spawn(Position{ 1, 2 });

    world.add_components(entity, Velocity{ 3, 4 });

    auto* archetype = entity_archetype(world, entity);
    require(archetype != nullptr);
    require(archetype->template has<Position, Velocity>());
    size_t moved_count = 0;
    for (auto [position, velocity] : view_of<Position&, Velocity&>(*archetype)) {
        require(position.x == 1 && position.y == 2);
        require(velocity.x == 3 && velocity.y == 4);
        ++moved_count;
    }
    require(moved_count == 1);

    world.remove_components<Velocity>(entity);

    archetype = entity_archetype(world, entity);
    require(archetype != nullptr);
    require(archetype->template has<Position>());
    require_false(archetype->template has<Velocity>());
    size_t position_only_count = 0;
    for (auto [position] : view_of<Position&>(*archetype)) {
        require(position.x == 1 && position.y == 2);
        ++position_only_count;
    }
    require(position_only_count == 1);

    world.remove_components<Position>(entity);

    const auto index = static_cast<size_t>(entity);
    auto& entities   = world_accessor::entities(world);
    require(index < entities.size());
    require(entities[index].first == entity);
    require(entities[index].second == nullptr);
}

void test_add_components_without_values() {
    basic_world<world_descriptor_t<>> world;
    const auto entity = world.spawn(Position{ 2, 3 });

    world.add_components<Velocity, Tag>(entity);

    auto* archetype = entity_archetype(world, entity);
    require(archetype != nullptr);
    require(archetype->template has<Position, Velocity, Tag>());

    size_t count = 0;
    for (auto [position, velocity] : view_of<Position&, Velocity&>(*archetype)) {
        require(position.x == 2 && position.y == 3);
        require(velocity.x == 0 && velocity.y == 0);
        ++count;
    }
    require(count == 1);

    world.remove_components<Velocity, Tag>(entity);

    archetype = entity_archetype(world, entity);
    require(archetype != nullptr);
    require(archetype->template has<Position>());
    require_false(archetype->template has<Velocity>());
    require_false(archetype->template has<Tag>());
}

void test_overlap_add_and_missing_remove_are_noops() {
    basic_world<world_descriptor_t<>> world;
    const auto entity = world.spawn(Position{ 5, 6 });

    world.add_components(entity, Position{ 9, 9 }, Velocity{ 7, 8 });
    world.add_components<Position, Tag>(entity);

    auto* archetype = entity_archetype(world, entity);
    require(archetype != nullptr);
    require(archetype->template has<Position, Velocity, Tag>());
    size_t count = 0;
    for (auto [position, velocity] : view_of<Position&, Velocity&>(*archetype)) {
        require(position.x == 5 && position.y == 6);
        require(velocity.x == 7 && velocity.y == 8);
        ++count;
    }
    require(count == 1);

    world.remove_components<Tag>(entity);
    world.remove_components<Tag>(entity);

    count = 0;
    archetype = entity_archetype(world, entity);
    require(archetype != nullptr);
    require(archetype->template has<Position, Velocity>());
    require_false(archetype->template has<Tag>());
    for (auto [position, velocity] : view_of<Position&, Velocity&>(*archetype)) {
        require(position.x == 5 && position.y == 6);
        require(velocity.x == 7 && velocity.y == 8);
        ++count;
    }
    require(count == 1);
}

void test_command_buffer_future_add_components() {
    basic_world<world_descriptor_t<>> world;
    command_buffer<> buffer;

    const auto future = buffer.spawn(Position{ 10, 11 });
    buffer.add_components(future, Velocity{ 12, 13 });
    buffer.add_components<Tag>(future);
    buffer.apply(world);

    auto& entities      = world_accessor::entities(world);
    auto* const archetype = entities[1].second;
    require(archetype != nullptr);
    require(archetype->template has<Position, Velocity, Tag>());
    size_t count = 0;
    for (auto [position, velocity] : view_of<Position&, Velocity&>(*archetype)) {
        require(position.x == 10 && position.y == 11);
        require(velocity.x == 12 && velocity.y == 13);
        ++count;
    }
    require(count == 1);
}

void test_command_buffer_future_remove_components() {
    basic_world<world_descriptor_t<>> world;
    command_buffer<> buffer;

    const auto future = buffer.spawn(Position{ 20, 21 }, Velocity{ 22, 23 });
    buffer.remove_components<Velocity>(future);
    buffer.apply(world);

    auto& entities         = world_accessor::entities(world);
    auto* const archetype  = entities[1].second;
    require(archetype != nullptr);
    require(archetype->template has<Position>());
    require_false(archetype->template has<Velocity>());

    size_t count = 0;
    for (auto [position] : view_of<Position&>(*archetype)) {
        require(position.x == 20 && position.y == 21);
        ++count;
    }
    require(count == 1);
}

int main() {
    test_add_and_remove_components();
    neutron::println("world_base test: add/remove ok");
    test_add_components_without_values();
    neutron::println("world_base test: add default ok");
    test_overlap_add_and_missing_remove_are_noops();
    neutron::println("world_base test: overlap/noop ok");
    test_command_buffer_future_add_components();
    neutron::println("world_base test: command buffer ok");
    test_command_buffer_future_remove_components();
    neutron::println("world_base test: command buffer remove ok");
    return 0;
}
