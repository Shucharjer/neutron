#include <cstddef>
#include <neutron/ecs.hpp>

using namespace neutron;

template <query_filter<std::allocator<std::byte>>... Filters>
using querior = basic_query<std::allocator<std::byte>, 8, Filters...>;

template <>
constexpr bool as_component<int> = true;

void test_constructor() {
    {
        basic_world<decltype(world_desc)> world;
        querior<with<int&>> querior{ world };
    }

    {
        basic_world<decltype(world_desc)> world;
        querior<with<int&>> querior{ world };
        auto copy{ querior };
    }

    {
        basic_world<decltype(world_desc)> world;
        querior<with<int&>> querior{ world };
        auto move{ std::move(querior) };
    }
}

void test_get() {
    {
        basic_world<decltype(world_desc)> world;
        querior<with<int&>> querior{ world };
        for (auto [val] : querior.get()) {}
    }

    {
        basic_world<decltype(world_desc)> world;
        querior<with<int&>> querior{ world };
        for (auto [val] : querior.get<int>()) {}
    }

    {
        basic_world<decltype(world_desc)> world;
        querior<with<int&>> querior{ world };
        for (auto [ent, val] : querior.get_with_entity()) {}
    }

    {
        basic_world<decltype(world_desc)> world;
        querior<with<int&>> querior{ world };
        for (auto [ent, val] : querior.get_with_entity<int>()) {}
    }
}

int main() {
    test_constructor();
    test_get();

    return 0;
}
