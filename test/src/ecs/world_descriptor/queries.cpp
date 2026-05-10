#include "neutron/detail/ecs/world_descriptor.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_events.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_execution_frequency.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_execution_policy.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_max_buffer_count.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_max_concurrency.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_min_buffer_count.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_min_concurrency.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_render.hpp"
#include "neutron/ecs.hpp"

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
            static_assert(get_max_concurrency(desc) == 3);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<update, fn1, { fn2, after<fn1> }, fn3>;
            static_assert(get_max_concurrency(desc) == 2);
        }

        {
            constexpr auto desc =
                world_desc |
                add_systems<update, fn1, { fn2, before<fn1> }, fn3>;
            static_assert(get_max_concurrency(desc) == 2);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<update, fn1, { fn2, individual }, fn3>;
            static_assert(get_max_concurrency(desc) == 2);
        }

        {
            constexpr auto desc =
                world_desc |
                add_systems<
                    update, fn1, { fn2, individual }, { fn3, individual }>;
            static_assert(get_max_concurrency(desc) == 1);
        }

        {
            constexpr auto desc =
                world_desc |
                add_systems<
                    update, fn1, { fn2, individual }, { fn3, after<fn2> }>;
            static_assert(get_max_concurrency(desc) == 2);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<
                                 update, fn1, { fn2, individual, after<fn1> },
                                 { fn3, after<fn2> }>;
            static_assert(get_max_concurrency(desc) == 1);
        }

        {
            constexpr auto desc = world_desc |
                                  add_systems<update, fn1, fn2, fn3> |
                                  execute<individual>;
            static_assert(get_max_concurrency(desc) == 3);
        }

        void fn4();
        {
            constexpr auto desc =
                world_desc |
                add_systems<
                    update, fn1, { fn2, after<fn1> }, { fn3, after<fn1> }, fn4>;
            // (fn1, fn4) -> (fn2, fn3, [fn4])
            // because of fn4 may be unfinished
            static_assert(get_max_concurrency(desc) == 3);
        }
    }

    // get_max_buffer_count
    {
        void fn1(); // NOLINT
        void fn2(); // NOLINT
        void cmd1(commands);
        void cmd2(commands);
        void cmd3(commands);
        void cmd4(commands);
        void direct(direct_commands);

        {
            constexpr auto desc = world_desc;
            static_assert(get_max_buffer_count(desc) == 0);
        }

        {
            constexpr auto desc = world_desc | add_systems<update, fn1, fn2>;
            static_assert(get_max_buffer_count(desc) == 0);
        }

        {
            constexpr auto desc = world_desc | add_systems<update, cmd1>;
            static_assert(get_max_buffer_count(desc) == 1);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<update, cmd1, cmd2, cmd3>;
            static_assert(get_max_buffer_count(desc) == 3);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<
                                 update, cmd1, { cmd2, after<cmd1> },
                                 { cmd3, after<cmd1> }, cmd4>;
            static_assert(get_max_buffer_count(desc) == 3);
        }

        {
            constexpr auto desc =
                world_desc |
                add_systems<update, cmd1, { cmd2, after<cmd1> }, fn1, fn2>;
            static_assert(get_max_buffer_count(desc) == 1);
        }

        {
            constexpr auto desc =
                world_desc |
                add_systems<update, cmd1, { cmd2, individual }, cmd3>;
            static_assert(get_max_buffer_count(desc) == 2);
        }

        {
            constexpr auto desc =
                world_desc |
                add_systems<update, { cmd1, individual }, { cmd2, individual }>;
            static_assert(get_max_buffer_count(desc) == 1);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<update, direct, cmd1>;
            static_assert(get_max_buffer_count(desc) == 1);
        }

        {
            constexpr auto desc = world_desc | add_systems<update, cmd1> |
                                  add_systems<render, cmd2, cmd3>;
            static_assert(get_max_buffer_count(desc) == 2);
        }
    }

    // get_min_concurrency
    {
        void fn1(); // NOLINT
        void fn2(); // NOLINT
        void fn3(); // NOLINT
        void fn4(); // NOLINT

        {
            constexpr auto desc = world_desc;
            static_assert(get_min_concurrency(desc) == 1);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<update, fn1, fn2, fn3>;
            static_assert(get_min_concurrency(desc) == 3);
        }

        {
            constexpr auto desc =
                world_desc |
                add_systems<
                    update, fn1, { fn2, after<fn1> }, { fn3, after<fn1> }, fn4>;
            static_assert(get_max_concurrency(desc) == 3);
            static_assert(get_min_concurrency(desc) == 2);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<update, fn1, { fn2, individual }, fn3>;
            static_assert(get_min_concurrency(desc) == 2);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<
                                 update, { fn1, individual },
                                 { fn2, individual }, { fn3, individual }>;
            static_assert(get_min_concurrency(desc) == 1);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<
                                 update, fn1, { fn2, individual, after<fn1> },
                                 { fn3, after<fn2> }>;
            static_assert(get_min_concurrency(desc) == 1);
        }

        {
            constexpr auto desc = world_desc |
                                  add_systems<update, fn1, fn2, fn3> |
                                  execute<individual>;
            static_assert(get_min_concurrency(desc) == 3);
        }

        {
            constexpr auto desc = world_desc | add_systems<update, fn1, fn2> |
                                  add_systems<render, fn1, fn2, fn3>;
            static_assert(get_min_concurrency(desc) == 3);
        }
    }

    // get_min_buffer_count
    {
        void fn1();                   // NOLINT
        void fn2();                   // NOLINT
        void cmd1(commands);          // NOLINT
        void cmd2(commands);          // NOLINT
        void cmd3(commands);          // NOLINT
        void cmd4(commands);          // NOLINT
        void direct(direct_commands); // NOLINT

        {
            constexpr auto desc = world_desc;
            static_assert(get_min_buffer_count(desc) == 0);
        }

        {
            constexpr auto desc = world_desc | add_systems<update, fn1, fn2>;
            static_assert(get_min_buffer_count(desc) == 0);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<update, cmd1, cmd2, cmd3>;
            static_assert(get_min_buffer_count(desc) == 3);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<
                                 update, cmd1, { cmd2, after<cmd1> },
                                 { cmd3, after<cmd1> }, cmd4>;
            static_assert(get_max_buffer_count(desc) == 3);
            static_assert(get_min_buffer_count(desc) == 2);
        }

        {
            constexpr auto desc =
                world_desc |
                add_systems<update, cmd1, { cmd2, after<cmd1> }, fn1, fn2>;
            static_assert(get_min_buffer_count(desc) == 1);
        }

        {
            constexpr auto desc =
                world_desc |
                add_systems<update, cmd1, { cmd2, individual }, cmd3>;
            static_assert(get_min_buffer_count(desc) == 2);
        }

        {
            constexpr auto desc =
                world_desc |
                add_systems<update, { cmd1, individual }, { cmd2, individual }>;
            static_assert(get_min_buffer_count(desc) == 1);
        }

        {
            constexpr auto desc =
                world_desc | add_systems<update, direct, cmd1>;
            static_assert(get_min_buffer_count(desc) == 1);
        }

        {
            constexpr auto desc = world_desc | add_systems<update, cmd1> |
                                  add_systems<render, cmd2, cmd3>;
            static_assert(get_min_buffer_count(desc) == 2);
        }
    }

    return 0;
}
