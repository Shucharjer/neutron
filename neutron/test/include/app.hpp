#pragma once
#include <cstddef>
#include <memory_resource>
#include <tuple>
#include <vector>
#include <exec/static_thread_pool.hpp>
#include <neutron/ecs.hpp>
#include <neutron/metafn.hpp>

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

        auto thread_pool   = exec::static_thread_pool{};
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

        // startup
        call<pre_startup>(sch, command_buffers, worlds);
        call<startup>(sch, command_buffers, worlds);
        call<post_startup>(sch, command_buffers, worlds);

        // update

        constexpr auto total_frames = 4;
        for (auto frame = 0; frame < total_frames; ++frame) {
            call<pre_update>(sch, command_buffers, worlds);
            call<update>(sch, command_buffers, worlds);
            call<post_update>(sch, command_buffers, worlds);
        }

        // shutdown
        call<shutdown>(sch, command_buffers, worlds);
    }
};

class mmapp {
public:
    using config_type = std::tuple<>;

    static mmapp create() { return {}; }

    template <auto... Worlds>
    void run() {}
};
