#include <neutron/detail/ecs/params/construct/local.hpp>
#include <neutron/ecs.hpp>
#include <neutron/print.hpp>
#include "require.hpp"

using namespace neutron;
using enum stage;

void hint(local<int> loc) {}

int main() {
    {
        constexpr auto desc = world_desc | add_systems<update, hint>;
        auto world          = make_world<desc>();
        {
            auto loc =
                construct_from_world<update, hint, local<int>>(world);
            auto& [var] = loc;
            require_or_return(var == 0, 1);
            var = 32;
        }
        {
            auto loc =
                construct_from_world<update, hint, local<int>>(world);
            auto& [var] = loc;
            require_or_return(var == 32, 1);
        }
    }
    return 0;
}
