#include <concepts>
#include <type_traits>
#include <neutron/detail/ecs/compile-time/collect_params.hpp>
#include <neutron/ecs.hpp>

using namespace neutron;
using enum stage;

template <>
constexpr bool ::neutron::as_component<int> = true;

void noop() {
    //
}

void query_int(query<with<int>>) {
    //
}

void query_int_with_cmds(query<with<int>>, commands cmds) {
    //
}

int main() {
    {
        constexpr auto desc =
            world_desc | enable_events | add_systems<events, noop>;
        using deducing = query_cache_of<std::remove_cvref_t<decltype(desc)>>;
        static_assert(std::same_as<deducing, type_list<>>);
    }

    {
        constexpr auto desc =
            world_desc | enable_events | add_systems<events, query_int>;
        using deducing = query_cache_of<std::remove_cvref_t<decltype(desc)>>;
        static_assert(std::same_as<deducing, type_list<query<with<int>>>>);
    }

    {
        constexpr auto desc = world_desc | enable_events |
                              add_systems<events, query_int_with_cmds>;
        using deducing = query_cache_of<std::remove_cvref_t<decltype(desc)>>;
        static_assert(std::same_as<deducing, type_list<query<with<int>>>>);
    }

    return 0;
}
