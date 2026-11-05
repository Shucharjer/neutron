#include <neutron/detail/ecs/params/construct/local.hpp>
#include <neutron/ecs.hpp>
#include <neutron/print.hpp>
#include "require.hpp"

using namespace neutron;
using enum stage;

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

int main() {
    test_local_is_value_initialized_and_persists();
    return neutron_test::result();
}
