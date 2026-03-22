#pragma once
#ifdef ATOM_EXECUTION
    #undef ATOM_EXECUTION
#endif
#include <cstddef>
#include <memory_resource>
#include <tuple>
#include <vector>
#include <neutron/ecs.hpp>
#include <neutron/execution.hpp>
#include <neutron/metafn.hpp>
#include "thread_pool.hpp"

using commands = neutron::basic_commands<std::pmr::polymorphic_allocator<>>;

class myapp {
public:
    using config_type = std::tuple<>;
    static myapp create() { return {}; }
    template <auto World> // single world
    void run() {
        using namespace neutron;
        using namespace neutron::execution;
        using enum stage;

        // create allocator
        constexpr auto bufsize = 1024 * 1024 * 1;
        std::vector<std::byte> buf(bufsize);
        auto upstream     = std::pmr::monotonic_buffer_resource{};
        auto pool         = std::pmr::synchronized_pool_resource{ &upstream };
        using allocator_t = std::pmr::polymorphic_allocator<>;
        auto alloc        = allocator_t{ &pool };

        // create execution environment

        auto thread_pool   = thread_pool_for_test::thread_pool{};
        scheduler auto sch = thread_pool.get_scheduler();

        // make command buffers

        using command_buffer_t = command_buffer<allocator_t>;
        using _cmdbuf_allocator =
            std::pmr::polymorphic_allocator<command_buffer_t>;
        const auto concurrency = thread_pool.available_parallelism();
        std::pmr::vector<command_buffer_t> command_buffers(
            concurrency, _cmdbuf_allocator{ alloc });

        // make worlds

        auto worlds = make_worlds<World>(alloc);

        auto rt = make_runtime(sch, command_buffers, worlds);
        rt.run();
    }
};
