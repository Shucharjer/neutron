// Basic tests for neutron::archetype: creation, emplace/view, erase, reserve,
// pmr
#include <array>
#include <memory_resource>
#include <neutron/ecs.hpp>
#include "require.hpp"

using namespace neutron;

// Test component types
struct Position {
    using component_concept = neutron::component_t;
    float x{ 0 }, y{ 0 };
};
struct Velocity {
    using component_concept = neutron::component_t;
    float vx{ 0 }, vy{ 0 };
};
struct TagEmpty {
    using component_concept = neutron::component_t;
};
struct Tracker {
    using component_concept       = neutron::component_t;
    static inline int ctor        = 0;
    static inline int dtor        = 0;
    static inline int move_ctor   = 0;
    static inline int move_assign = 0;
    int v{ 0 };
    Tracker() noexcept { ++ctor; }
    explicit Tracker(int x) noexcept : v(x) { ++ctor; }
    Tracker(const Tracker&) = default;
    Tracker(Tracker&& other) noexcept : v(other.v) { ++move_ctor; }
    Tracker& operator=(const Tracker&) = default;
    Tracker& operator=(Tracker&& other) noexcept {
        v = other.v;
        ++move_assign;
        return *this;
    }
    ~Tracker() noexcept { ++dtor; }
};

struct SlowDefault {
    using component_concept = neutron::component_t;
    static inline int alive = 0;
    int value{ 0 };

    SlowDefault() { ++alive; }
    SlowDefault(const SlowDefault&) { ++alive; }
    SlowDefault(SlowDefault&&) noexcept { ++alive; }
    SlowDefault& operator=(const SlowDefault&) = default;
    SlowDefault& operator=(SlowDefault&&) noexcept = default;
    ~SlowDefault() noexcept { --alive; }
};

struct SlowCopy {
    using component_concept = neutron::component_t;
    static inline int alive = 0;
    int value{ 0 };

    SlowCopy() { ++alive; }
    explicit SlowCopy(int x) noexcept : value(x) { ++alive; }
    SlowCopy(const SlowCopy& that) : value(that.value) { ++alive; }
    SlowCopy(SlowCopy&& that) noexcept : value(that.value) { ++alive; }
    SlowCopy& operator=(const SlowCopy&) = default;
    SlowCopy& operator=(SlowCopy&&) noexcept = default;
    ~SlowCopy() noexcept { --alive; }
};

using archetype_t = archetype<std::allocator<std::byte>>;

int require_position(
    archetype_t& arche, float x, float y, size_t expected_count = 1) {
    size_t count = 0;
    for (auto [position] : view_of<Position&>(arche)) {
        require_or_return((position.x == x && position.y == y), 1);
        ++count;
    }
    require_or_return((count == expected_count), 1);
    return 0;
}

int require_position_velocity(
    archetype_t& arche, float px, float py, float vx, float vy,
    size_t expected_count = 1) {
    size_t count = 0;
    for (auto [position, velocity] : view_of<Position&, Velocity&>(arche)) {
        require_or_return((position.x == px && position.y == py), 1);
        require_or_return((velocity.vx == vx && velocity.vy == vy), 1);
        ++count;
    }
    require_or_return((count == expected_count), 1);
    return 0;
}

int test_basics() {
    archetype_t arche{ type_spreader<Position, Velocity, TagEmpty>{} };

    require_or_return((arche.kinds() == 3), 1);
    require_or_return((arche.size() == 0), 1);
    require_or_return((arche.capacity() >= 64), 1);
    require_or_return((arche.has<Position>()), 1);
    require_or_return((arche.has<Velocity, TagEmpty>()), 1);
    require_false_or_return((arche.has<int>()), 1);
    return 0;
}

int test_emplace_accepts_declared_component_order() {
    archetype_t arche{ type_spreader<Position, Velocity>{} };

    arche.emplace(entity_t{ 1 }, Position{ 1, 2 }, Velocity{ 3, 4 });

    require_or_return((arche.size() == 1), 1);
    if (const auto ret = require_position_velocity(arche, 1, 2, 3, 4);
        ret != 0) {
        return ret;
    }
    return 0;
}

int test_emplace_accepts_arbitrary_component_order() {
    archetype_t arche{ type_spreader<Position, Velocity>{} };

    arche.emplace(entity_t{ 2 }, Velocity{ 7, 8 }, Position{ 5, 6 });

    require_or_return((arche.size() == 1), 1);
    if (const auto ret = require_position_velocity(arche, 5, 6, 7, 8);
        ret != 0) {
        return ret;
    }
    return 0;
}

int test_at_accepts_arbitrary_component_order_and_references() {
    archetype_t arche{ type_spreader<Position, Velocity>{} };

    arche.emplace(entity_t{ 3 }, Velocity{ 7, 8 }, Position{ 5, 6 });

    auto ordered   = arche.at<Position&, Velocity&>(entity_t{ 3 });
    auto reordered = arche.at<Velocity&, Position&>(entity_t{ 3 });

    auto& position = std::get<0>(ordered);
    auto& velocity = std::get<1>(ordered);
    auto& velocity2 = std::get<0>(reordered);
    auto& position2 = std::get<1>(reordered);

    require_or_return((&position == &position2), 1);
    require_or_return((&velocity == &velocity2), 1);
    require_or_return((position.x == 5 && position.y == 6), 1);
    require_or_return((velocity.vx == 7 && velocity.vy == 8), 1);

    position2.x = 9;
    velocity2.vy = 10;

    if (const auto ret = require_position_velocity(arche, 9, 6, 7, 10);
        ret != 0) {
        return ret;
    }
    return 0;
}

int test_view_iterates_inserted_entities() {
    archetype_t arche{ type_spreader<Position, Velocity>{} };

    arche.emplace(entity_t{ 1 }, Position{ 1, 2 }, Velocity{ 3, 4 });
    arche.emplace(entity_t{ 2 }, Velocity{ 7, 8 }, Position{ 5, 6 });

    require_or_return((arche.size() == 2), 1);

    auto view    = view_of<Position&, Velocity&>(arche);
    size_t index = 0;
    for (auto [position, velocity] : view) {
        if (index == 0) {
            require_or_return((position.x == 1 && position.y == 2), 1);
            require_or_return((velocity.vx == 3 && velocity.vy == 4), 1);
        } else if (index == 1) {
            require_or_return((position.x == 5 && position.y == 6), 1);
            require_or_return((velocity.vx == 7 && velocity.vy == 8), 1);
        }
        ++index;
    }
    require_or_return((index == 2), 1);
    return 0;
}

int test_emplace_range_default_constructs_all_entities() {
    archetype_t arche{ type_spreader<Position, Velocity>{} };
    const std::array entities{
        entity_t{ 10 }, entity_t{ 11 }, entity_t{ 12 }, entity_t{ 13 }
    };

    arche.template emplace<Position, Velocity>(entities);

    require_or_return((arche.size() == entities.size()), 1);
    if (const auto ret =
            require_position_velocity(arche, 0, 0, 0, 0, entities.size());
        ret != 0) {
        return ret;
    }
    return 0;
}

int test_emplace_range_values_accept_arbitrary_component_order() {
    archetype_t arche{ type_spreader<Position, Velocity>{} };
    const std::array entities{ entity_t{ 20 }, entity_t{ 21 }, entity_t{ 22 } };

    arche.emplace(entities, Velocity{ 7, 8 }, Position{ 5, 6 });

    require_or_return((arche.size() == entities.size()), 1);
    if (const auto ret =
            require_position_velocity(arche, 5, 6, 7, 8, entities.size());
        ret != 0) {
        return ret;
    }
    return 0;
}

int test_emplace_range_instantiates_throwing_paths() {
    SlowDefault::alive = 0;
    {
        archetype<std::allocator<std::byte>> arche{
            type_spreader<SlowDefault, Position>{}
        };
        const std::array entities{ entity_t{ 30 }, entity_t{ 31 } };

        arche.template emplace<SlowDefault, Position>(entities);

        require_or_return((arche.size() == entities.size()), 1);
        require_or_return(
            (SlowDefault::alive == static_cast<int>(entities.size())), 1);
    }
    require_or_return((SlowDefault::alive == 0), 1);

    SlowCopy::alive = 0;
    {
        archetype<std::allocator<std::byte>> arche{
            type_spreader<Position, SlowCopy>{}
        };
        const std::array entities{ entity_t{ 40 }, entity_t{ 41 }, entity_t{ 42 } };
        SlowCopy component{ 9 };

        arche.emplace(entities, component, Position{ 1, 2 });

        require_or_return((arche.size() == entities.size()), 1);
        require_or_return(
            (SlowCopy::alive == static_cast<int>(entities.size() + 1)), 1);

        size_t count = 0;
        for (auto [position, copy] : view_of<Position&, SlowCopy&>(arche)) {
            require_or_return((position.x == 1 && position.y == 2), 1);
            require_or_return((copy.value == 9), 1);
            ++count;
        }
        require_or_return((count == entities.size()), 1);
    }
    require_or_return((SlowCopy::alive == 0), 1);
    return 0;
}

int test_erase_and_entities() {
    Tracker::ctor = Tracker::dtor = Tracker::move_ctor = Tracker::move_assign =
        0;
    archetype<std::allocator<std::byte>> arche{
        type_spreader<Tracker, Position>{}
    };

    arche.emplace(entity_t{ 1 }, Tracker{ 1 }, Position{ 1, 1 });
    arche.emplace(entity_t{ 2 }, Tracker{ 2 }, Position{ 2, 2 });
    arche.emplace(entity_t{ 3 }, Tracker{ 3 }, Position{ 3, 3 });
    require_or_return((arche.size() == 3), 1);

    // erase middle; last should move into the gap
    arche.erase(entity_t{ 2 });
    require_or_return((arche.size() == 2), 1);
    return 0;
}

int test_reserve_and_relocate() {
    archetype_t arche{ type_spreader<Position, Velocity>{} };
    const size_t initial = arche.capacity();
    arche.reserve(initial * 2);
    require_or_return((arche.capacity() >= initial * 2), 1);

    // Fill across multiple relocations and validate data integrity
    const size_t N = initial * 3 + 5;
    for (size_t i = 0; i < N; ++i) {
        arche.emplace(
            static_cast<entity_t>(i + 1), Position{ float(i), float(i + 1) },
            Velocity{ float(2 * i), float(2 * i + 1) });
    }
    require_or_return((arche.size() == N), 1);
    return 0;
}

int test_pmr_archetype() {
    std::pmr::unsynchronized_pool_resource pool;
    std::pmr::polymorphic_allocator<> alloc{ &pool };
    pmr::archetype arche{ type_spreader<Position, TagEmpty>{}, alloc };
    require_or_return((arche.kinds() == 2), 1);
    arche.emplace(entity_t{ 42 }, Position{ 4, 2 }, TagEmpty{});
    require_or_return((arche.size() == 1), 1);
    return 0;
}

int test_transfer_adds_component_with_value() {
    archetype_t source{ type_spreader<Position>{} };
    archetype_t target{ source, add_components_t<Velocity>{} };

    source.emplace(entity_t{ 1 }, Position{ 1, 2 });
    source.transfer(
        entity_t{ 1 }, target, add_components_t<Velocity>{}, Velocity{ 3, 4 });

    require_or_return((source.empty()), 1);
    require_or_return((target.size() == 1), 1);
    if (const auto ret = require_position_velocity(target, 1, 2, 3, 4);
        ret != 0) {
        return ret;
    }
    return 0;
}

int test_transfer_adds_components_with_default_values() {
    archetype_t source{ type_spreader<Position>{} };
    archetype_t target{ source, add_components_t<Velocity, TagEmpty>{} };

    source.emplace(entity_t{ 2 }, Position{ 5, 6 });
    source.transfer(
        entity_t{ 2 }, target, add_components_t<Velocity, TagEmpty>{});

    require_or_return((source.empty()), 1);
    require_or_return((target.size() == 1), 1);
    require_or_return((target.has<Position, Velocity, TagEmpty>()), 1);
    if (const auto ret = require_position_velocity(target, 5, 6, 0, 0);
        ret != 0) {
        return ret;
    }
    return 0;
}

int test_transfer_removes_component() {
    archetype_t source{ type_spreader<Position, Velocity>{} };
    archetype_t target{ source, remove_components_t<Velocity>{} };

    source.emplace(entity_t{ 3 }, Position{ 7, 8 }, Velocity{ 9, 10 });
    source.transfer(entity_t{ 3 }, target);

    require_or_return((source.empty()), 1);
    require_or_return((target.size() == 1), 1);
    require_or_return((target.has<Position>()), 1);
    require_false_or_return((target.has<Velocity>()), 1);
    if (const auto ret = require_position(target, 7, 8); ret != 0) {
        return ret;
    }
    return 0;
}

int test_transfer_removes_empty_tag() {
    archetype_t source{ type_spreader<Position, TagEmpty, Velocity>{} };
    archetype_t target{ source, remove_components_t<TagEmpty>{} };

    source.emplace(
        entity_t{ 3 }, Position{ 7, 8 }, Velocity{ 9, 10 }, TagEmpty{});
    source.transfer(entity_t{ 3 }, target);

    require_or_return((source.empty()), 1);
    require_or_return((target.size() == 1), 1);
    require_or_return((target.has<Position>()), 1);
    require_or_return((target.has<Velocity>()), 1);
    require_false_or_return((target.has<TagEmpty>()), 1);
    if (const auto ret = require_position_velocity(target, 7, 8, 9, 10);
        ret != 0) {
        return ret;
    }
    return 0;
}

int main() {
    if (const auto ret = test_basics(); ret != 0) {
        return ret;
    }
    if (const auto ret = test_emplace_accepts_declared_component_order();
        ret != 0) {
        return ret;
    }
    if (const auto ret = test_emplace_accepts_arbitrary_component_order();
        ret != 0) {
        return ret;
    }
    if (const auto ret = test_at_accepts_arbitrary_component_order_and_references();
        ret != 0) {
        return ret;
    }
    if (const auto ret = test_view_iterates_inserted_entities(); ret != 0) {
        return ret;
    }
    if (const auto ret = test_emplace_range_default_constructs_all_entities();
        ret != 0) {
        return ret;
    }
    if (const auto ret = test_emplace_range_values_accept_arbitrary_component_order();
        ret != 0) {
        return ret;
    }
    if (const auto ret = test_emplace_range_instantiates_throwing_paths();
        ret != 0) {
        return ret;
    }
    if (const auto ret = test_erase_and_entities(); ret != 0) {
        return ret;
    }
    if (const auto ret = test_reserve_and_relocate(); ret != 0) {
        return ret;
    }
    if (const auto ret = test_pmr_archetype(); ret != 0) {
        return ret;
    }
    if (const auto ret = test_transfer_adds_component_with_value(); ret != 0) {
        return ret;
    }
    if (const auto ret = test_transfer_adds_components_with_default_values();
        ret != 0) {
        return ret;
    }
    if (const auto ret = test_transfer_removes_component(); ret != 0) {
        return ret;
    }
    if (const auto ret = test_transfer_removes_empty_tag(); ret != 0) {
        return ret;
    }
    return 0;
}
