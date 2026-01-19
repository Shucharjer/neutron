// Basic tests for neutron::archetype: creation, emplace/view, erase, reserve,
// pmr
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

void test_basics() {
    archetype<std::allocator<std::byte>> arche{
        type_spreader<Position, Velocity, TagEmpty>{}
    };

    require(arche.kinds() == 3);
    require(arche.size() == 0);
    require(arche.capacity() >= 64);
    require(arche.has<Position>());
    require(arche.has<Velocity, TagEmpty>());
    require_false(arche.has<int>());
}

void test_emplace_and_view() {
    archetype<std::allocator<std::byte>> arche{
        type_spreader<Position, Velocity>{}
    };

    // emplace with values (order of args should not matter)
    arche.emplace(entity_t{ 1 }, Position{ 1, 2 }, Velocity{ 3, 4 });
    arche.emplace(entity_t{ 2 }, Velocity{ 7, 8 }, Position{ 5, 6 });

    require(arche.size() == 2);

    // get raw storage and validate values
    auto stor = arche.get<Position, Velocity>();
    auto* pos = reinterpret_cast<Position*>(stor[0]);
    auto* vel = reinterpret_cast<Velocity*>(stor[1]);
    require(pos[0].x == 1 && pos[0].y == 2);
    require(vel[0].vx == 3 && vel[0].vy == 4);
    require(pos[1].x == 5 && pos[1].y == 6);
    require(vel[1].vx == 7 && vel[1].vy == 8);

    // view iteration (by value); verify iteration order and values
    auto vw = arche.view<Position, Velocity>();
    {
        size_t i = 0;
        for (auto [p, v] : vw) {
            if (i == 0) {
                require(p.x == 1 && p.y == 2);
                require(v.vx == 3 && v.vy == 4);
            } else if (i == 1) {
                require(p.x == 5 && p.y == 6);
                require(v.vx == 7 && v.vy == 8);
            }
            ++i;
        }
        require(i == 2);
    }
}

void test_erase_and_entities() {
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

    auto stor = arche.get<Tracker>();
    auto* tr  = reinterpret_cast<Tracker*>(stor[0]);
    // After erase of entity 2, entity 3 should be relocated into index 1
    require((tr[0].v == 1 && tr[1].v == 3));

    // internal eview exposes contiguous entity array; validate mapping updated
    neutron::_view::eview<Tracker> ev{ arche };
    auto ents = ev.entities();
    require(ents.size() == 2);
    // Order should correspond to storage indices [0, 1]
    require(ents[0] == entity_t{ 1 });
    require(ents[1] == entity_t{ 3 });

    // Tracker destructors invoked for the removed element (source of
    // move-assign)
    require(Tracker::dtor >= 1);
}

void test_reserve_and_relocate() {
    archetype<std::allocator<std::byte>> arche{
        type_spreader<Position, Velocity>{}
    };
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
    auto stor = arche.get<Position, Velocity>();
    auto* pos = reinterpret_cast<Position*>(stor[0]);
    auto* vel = reinterpret_cast<Velocity*>(stor[1]);
    for (size_t i = 0; i < N; ++i) {
        require(pos[i].x == float(i) && pos[i].y == float(i + 1));
        require(vel[i].vx == float(2 * i) && vel[i].vy == float(2 * i + 1));
    }
}

void test_pmr_archetype() {
    std::pmr::unsynchronized_pool_resource pool;
    std::pmr::polymorphic_allocator<> alloc{ &pool };
    pmr::archetype arche{ type_spreader<Position, TagEmpty>{}, alloc };
    require(arche.kinds() == 2);
    arche.emplace(entity_t{ 42 }, Position{ 4, 2 }, TagEmpty{});
    require(arche.size() == 1);
}

int main() {
    test_basics();
    neutron::println("archetype test: basics ok");
    test_emplace_and_view();
    neutron::println("archetype test: emplace/view ok");
    test_erase_and_entities();
    neutron::println("archetype test: erase/entities ok");
    test_reserve_and_relocate();
    neutron::println("archetype test: reserve/relocate ok");
    test_pmr_archetype();
    neutron::println("archetype test: pmr ok");
    return 0;
}
