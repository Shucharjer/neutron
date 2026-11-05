// Basic tests for neutron::archetype: creation, emplace/view, erase, reserve,
// pmr
#include <array>
#include <cstddef>
#include <memory_resource>
#include <neutron/ecs.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

struct Position {
    using component_concept = neutron::component_tag;
    float x{ 0 }, y{ 0 };
};
struct Velocity {
    using component_concept = neutron::component_tag;
    float vx{ 0 }, vy{ 0 };
};
struct TagEmpty {
    using component_concept = neutron::component_tag;
};
struct Tracker {
    using component_concept       = neutron::component_tag;
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
    using component_concept = neutron::component_tag;
    static inline int alive = 0;
    int value{ 0 };

    SlowDefault() { ++alive; }
    SlowDefault(const SlowDefault&) { ++alive; }
    SlowDefault(SlowDefault&&) noexcept { ++alive; }
    SlowDefault& operator=(const SlowDefault&)     = default;
    SlowDefault& operator=(SlowDefault&&) noexcept = default;
    ~SlowDefault() noexcept { --alive; }
};

struct SlowCopy {
    using component_concept = neutron::component_tag;
    static inline int alive = 0;
    int value{ 0 };

    SlowCopy() { ++alive; }
    explicit SlowCopy(int x) noexcept : value(x) { ++alive; }
    SlowCopy(const SlowCopy& that) : value(that.value) { ++alive; }
    SlowCopy(SlowCopy&& that) noexcept : value(that.value) { ++alive; }
    SlowCopy& operator=(const SlowCopy&)     = default;
    SlowCopy& operator=(SlowCopy&&) noexcept = default;
    ~SlowCopy() noexcept { --alive; }
};

using archetype_t = archetype<std::allocator<std::byte>>;

void require_position(
    archetype_t& arche, float x, float y, size_t expected_count = 1) {
    using position_query = query<with<Position&>>;

    size_t count = 0;
    auto slice   = slice_of<Position>(arche);
    for (auto [position] : typename position_query::view_t{ slice }) {
        require(position.x == x);
        require(position.y == y);
        ++count;
    }
    require(count == expected_count);
}

void require_position_velocity(
    archetype_t& arche, float px, float py, float vx, float vy,
    size_t expected_count = 1) {
    using position_velocity_query = query<with<Position&, Velocity&>>;

    size_t count = 0;
    auto slice   = slice_of<Position, Velocity>(arche);
    for (auto [position, velocity] :
         typename position_velocity_query::view_t{ slice }) {
        require(position.x == px);
        require(position.y == py);
        require(velocity.vx == vx);
        require(velocity.vy == vy);
        ++count;
    }
    require(count == expected_count);
}

void test_new_archetype_is_empty_and_knows_its_components() {
    constexpr size_t min_initial_capacity = 64;

    archetype_t arche{ type_spreader<Position, Velocity, TagEmpty>{} };

    require(arche.kinds() == 3);
    require(arche.size() == 0);
    require(arche.capacity() >= min_initial_capacity);
    require(arche.has<Position>());
    require(arche.has<Velocity, TagEmpty>());
    require_false(arche.has<int>());
}

void test_emplace_accepts_declared_component_order() {
    archetype_t arche{ type_spreader<Position, Velocity>{} };

    arche.emplace(entity_t{ 1 }, Position{ 1, 2 }, Velocity{ 3, 4 });

    require(arche.size() == 1);
    require_position_velocity(arche, 1, 2, 3, 4);
}

void test_emplace_accepts_arbitrary_component_order() {
    archetype_t arche{ type_spreader<Position, Velocity>{} };

    arche.emplace(entity_t{ 2 }, Velocity{ 7, 8 }, Position{ 5, 6 });

    require(arche.size() == 1);
    require_position_velocity(arche, 5, 6, 7, 8);
}

void test_at_accepts_arbitrary_component_order_and_references() {
    archetype_t arche{ type_spreader<Position, Velocity>{} };
    arche.emplace(entity_t{ 3 }, Velocity{ 7, 8 }, Position{ 5, 6 });

    auto ordered    = arche.at<Position&, Velocity&>(entity_t{ 3 });
    auto reordered  = arche.at<Velocity&, Position&>(entity_t{ 3 });
    auto& position  = std::get<0>(ordered);
    auto& velocity  = std::get<1>(ordered);
    auto& velocity2 = std::get<0>(reordered);
    auto& position2 = std::get<1>(reordered);

    require(&position == &position2);
    require(&velocity == &velocity2);
    require(position.x == 5);
    require(position.y == 6);
    require(velocity.vx == 7);
    require(velocity.vy == 8);

    // Writes through the returned references reach the stored components.
    position2.x  = 9;
    velocity2.vy = 10;

    require_position_velocity(arche, 9, 6, 7, 10);
}

void test_emplace_range_default_constructs_all_entities() {
    archetype_t arche{ type_spreader<Position, Velocity>{} };
    const std::array entities{ entity_t{ 10 }, entity_t{ 11 }, entity_t{ 12 },
                               entity_t{ 13 } };

    arche.template emplace<Position, Velocity>(entities);

    require(arche.size() == entities.size());
    require_position_velocity(arche, 0, 0, 0, 0, entities.size());
}

void test_emplace_range_values_accept_arbitrary_component_order() {
    archetype_t arche{ type_spreader<Position, Velocity>{} };
    const std::array entities{ entity_t{ 20 }, entity_t{ 21 }, entity_t{ 22 } };

    arche.emplace(entities, Velocity{ 7, 8 }, Position{ 5, 6 });

    require(arche.size() == entities.size());
    require_position_velocity(arche, 5, 6, 7, 8, entities.size());
}

void test_emplace_range_with_default_ctor_that_may_throw() {
    SlowDefault::alive = 0;
    {
        archetype<std::allocator<std::byte>> arche{
            type_spreader<SlowDefault, Position>{}
        };
        const std::array entities{ entity_t{ 30 }, entity_t{ 31 } };

        arche.template emplace<SlowDefault, Position>(entities);

        require(arche.size() == entities.size());
        require(SlowDefault::alive == static_cast<int>(entities.size()));
    }
    require(SlowDefault::alive == 0);
}

void test_emplace_range_with_copy_ctor_that_may_throw() {
    using position_copy_query = query<with<Position&, SlowCopy&>>;

    SlowCopy::alive = 0;
    {
        archetype<std::allocator<std::byte>> arche{
            type_spreader<Position, SlowCopy>{}
        };
        const std::array entities{ entity_t{ 40 }, entity_t{ 41 },
                                   entity_t{ 42 } };
        SlowCopy component{ 9 };

        arche.emplace(entities, component, Position{ 1, 2 });

        require(arche.size() == entities.size());
        // One copy per entity plus the local source.
        require(SlowCopy::alive == static_cast<int>(entities.size() + 1));

        auto slice = slice_of<Position, SlowCopy>(arche);
        require(slice.size() == entities.size());
        // Iterating below dereferences these pointers.
        require_or_return(slice.entities() != nullptr, );
        const auto& buffers = slice.buffers();
        require_or_return(std::get<0>(buffers) != nullptr, );
        require_or_return(std::get<1>(buffers) != nullptr, );

        size_t count = 0;
        for (auto [position, copy] :
             typename position_copy_query::view_t{ slice }) {
            require(position.x == 1);
            require(position.y == 2);
            require(copy.value == 9);
            ++count;
        }
        require(count == entities.size());
    }
    require(SlowCopy::alive == 0);
}

void test_erase_removes_one_entity() {
    Tracker::ctor = Tracker::dtor = Tracker::move_ctor = Tracker::move_assign =
        0;
    archetype<std::allocator<std::byte>> arche{
        type_spreader<Tracker, Position>{}
    };
    arche.emplace(entity_t{ 1 }, Tracker{ 1 }, Position{ 1, 1 });
    arche.emplace(entity_t{ 2 }, Tracker{ 2 }, Position{ 2, 2 });
    arche.emplace(entity_t{ 3 }, Tracker{ 3 }, Position{ 3, 3 });
    require(arche.size() == 3);

    // erase middle; last should move into the gap
    arche.erase(entity_t{ 2 });

    require(arche.size() == 2);
}

void test_reserve_grows_capacity_and_emplace_relocates() {
    archetype_t arche{ type_spreader<Position, Velocity>{} };
    const size_t initial = arche.capacity();

    arche.reserve(initial * 2);

    require(arche.capacity() >= initial * 2);

    // Fill across multiple relocations and validate data integrity
    const size_t N = initial * 3 + 5;
    for (size_t i = 0; i < N; ++i) {
        arche.emplace(
            static_cast<entity_t>(i + 1), Position{ float(i), float(i + 1) },
            Velocity{ float(2 * i), float(2 * i + 1) });
    }

    require(arche.size() == N);
}

void test_pmr_archetype_uses_polymorphic_allocator() {
    std::pmr::unsynchronized_pool_resource pool;
    std::pmr::polymorphic_allocator<> alloc{ &pool };
    pmr::archetype arche{ type_spreader<Position, TagEmpty>{}, alloc };

    require(arche.kinds() == 2);

    arche.emplace(entity_t{ 42 }, Position{ 4, 2 }, TagEmpty{});

    require(arche.size() == 1);
}

void test_transfer_adds_component_with_value() {
    archetype_t source{ type_spreader<Position>{} };
    archetype_t target{ source, add_components_t<Velocity>{} };
    source.emplace(entity_t{ 1 }, Position{ 1, 2 });

    source.transfer(
        entity_t{ 1 }, target, add_components_t<Velocity>{}, Velocity{ 3, 4 });

    require(source.empty());
    require(target.size() == 1);
    require_position_velocity(target, 1, 2, 3, 4);
}

void test_transfer_adds_components_with_default_values() {
    archetype_t source{ type_spreader<Position>{} };
    archetype_t target{ source, add_components_t<Velocity, TagEmpty>{} };
    source.emplace(entity_t{ 2 }, Position{ 5, 6 });

    source.transfer(
        entity_t{ 2 }, target, add_components_t<Velocity, TagEmpty>{});

    require(source.empty());
    require(target.size() == 1);
    require(target.has<Position, Velocity, TagEmpty>());
    require_position_velocity(target, 5, 6, 0, 0);
}

void test_transfer_removes_component() {
    archetype_t source{ type_spreader<Position, Velocity>{} };
    archetype_t target{ source, remove_components_t<Velocity>{} };
    source.emplace(entity_t{ 3 }, Position{ 7, 8 }, Velocity{ 9, 10 });

    source.transfer(entity_t{ 3 }, target);

    require(source.empty());
    require(target.size() == 1);
    require(target.has<Position>());
    require_false(target.has<Velocity>());
    require_position(target, 7, 8);
}

void test_transfer_removes_empty_tag() {
    archetype_t source{ type_spreader<Position, TagEmpty, Velocity>{} };
    archetype_t target{ source, remove_components_t<TagEmpty>{} };
    source.emplace(
        entity_t{ 3 }, Position{ 7, 8 }, Velocity{ 9, 10 }, TagEmpty{});

    source.transfer(entity_t{ 3 }, target);

    require(source.empty());
    require(target.size() == 1);
    require(target.has<Position>());
    require(target.has<Velocity>());
    require_false(target.has<TagEmpty>());
    require_position_velocity(target, 7, 8, 9, 10);
}

} // namespace

int main() {
    test_new_archetype_is_empty_and_knows_its_components();
    test_emplace_accepts_declared_component_order();
    test_emplace_accepts_arbitrary_component_order();
    test_at_accepts_arbitrary_component_order_and_references();
    test_emplace_range_default_constructs_all_entities();
    test_emplace_range_values_accept_arbitrary_component_order();
    test_emplace_range_with_default_ctor_that_may_throw();
    test_emplace_range_with_copy_ctor_that_may_throw();
    test_erase_removes_one_entity();
    test_reserve_grows_capacity_and_emplace_relocates();
    test_pmr_archetype_uses_polymorphic_allocator();
    test_transfer_adds_component_with_value();
    test_transfer_adds_components_with_default_values();
    test_transfer_removes_component();
    test_transfer_removes_empty_tag();
    return neutron_test::result();
}
