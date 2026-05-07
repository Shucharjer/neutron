#include "neutron/detail/ecs/world_descriptor.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_events.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_execution_frequency.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_execution_policy.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_max_concurrency.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_render.hpp"
#include "neutron/ecs.hpp"

#include "neutron/detail/type_traits/is_convertible_to_function_pointer.hpp"

using namespace neutron;
using enum stage;

int main() {

    // get_events
    {
        {
            constexpr auto desc = world_desc;
            static_assert(!get_events(desc));
        }

        {
            constexpr auto desc = world_desc | enable_events;
            static_assert(get_events(desc));
        }
    }

    // get_render
    {
        {
            constexpr auto desc = world_desc;
            static_assert(!get_render(desc));
        }

        {
            constexpr auto desc = world_desc | enable_render;
            static_assert(get_render(desc));
        }
    }

    // get_execution_policy
    {
        {
            constexpr auto desc   = world_desc;
            constexpr auto policy = get_execution_policy(desc);
            static_assert(std::same_as<decltype(policy), const _group_t<0>>);
        }

        {
            constexpr auto desc   = world_desc | execute<group<1>>;
            constexpr auto policy = get_execution_policy(desc);
            static_assert(std::same_as<decltype(policy), const _group_t<1>>);
        }

        {
            constexpr auto desc   = world_desc | execute<individual>;
            constexpr auto policy = get_execution_policy(desc);
            static_assert(std::same_as<decltype(policy), const _individual_t>);
        }
    }

    // get_execution_frequency
    {
        {
            constexpr auto desc = world_desc;
            static_assert(get_execution_frequency(desc) == 0.0);
        }

        {
            constexpr double interval = 1.0 / 32;
            constexpr auto desc = world_desc | execute<frequency<interval>>;
            static_assert(get_execution_frequency(desc) == interval);
        }
    }

    // get_max_concurrency
    {
        void fn1();
        void fn2();
        void fn3();

        {
            constexpr auto desc = world_desc;
            static_assert(get_max_concurrency(desc) == 1);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<update, fn1, fn2, fn3>;
            // static_assert(get_max_concurrency(desc) == 3);
            static_assert(get_max_concurrency(desc) == 1);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<update, fn1, { fn2, after<fn1> }, fn3>;
            // static_assert(get_max_concurrency(desc) == 2);
            static_assert(get_max_concurrency(desc) == 1);
        }

        {
            constexpr auto desc =
                world_desc |
                add_systems<update, fn1, { fn2, before<fn1> }, fn3>;
            // static_assert(get_max_concurrency(desc) == 2);
            static_assert(get_max_concurrency(desc) == 1);
        }
    }

    return 0;
}
