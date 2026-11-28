// With modules, an implementation unit of neutron that imports std, see
// `internal_tests`.
#ifndef ATOM_USES_MODULES
    #include <neutron/detail/ecs/params/construct/local.hpp>
    #include <neutron/ecs.hpp>
    #include <neutron/print.hpp>
#endif
#include "require.hpp"

using namespace neutron;

// Not in the global namespace, where `shutdown` clashes with the winsock
// function declared by <windows.h>.
namespace {
using enum stage;
} // namespace

namespace {

void hint(local<int>) {}

void test_local_is_value_initialized_and_persists() {
    constexpr auto desc        = world_desc | add_systems<update, hint>;
    auto world                 = make_world<desc>();
    constexpr int stored_value = 32;

    {
        auto loc    = construct_from_world<update, hint, local<int>>(world);
        auto& [var] = loc;
        require(var == 0);
        var = stored_value;
    }

    auto loc    = construct_from_world<update, hint, local<int>>(world);
    auto& [var] = loc;
    require(var == stored_value);
}

} // namespace

extern "C++" int main() {
    test_local_is_value_initialized_and_persists();
    return neutron_test::result();
}
