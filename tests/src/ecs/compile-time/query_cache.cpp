// With modules, an implementation unit of neutron that imports std, see
// `internal_tests`.
#ifndef ATOM_USES_MODULES
    #include <concepts>
    #include <type_traits>
    #include <neutron/detail/ecs/compile-time/collect_params.hpp>
    #include <neutron/ecs.hpp>
#endif
#include "require.hpp"

using namespace neutron;

// Not in the global namespace, where `shutdown` clashes with the winsock
// function declared by <windows.h>.
namespace {
using enum stage;
} // namespace

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

extern "C++" int main() {
    // A system without queries contributes nothing to the cache.
    {
        constexpr auto desc =
            world_desc | enable_events | add_systems<events, noop>;
        using deducing = query_cache_of<std::remove_cvref_t<decltype(desc)>>;
        static_assert(std::same_as<deducing, type_list<>>);
    }

    // Each query parameter is cached; other parameters are ignored.
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

    return neutron_test::result();
}
