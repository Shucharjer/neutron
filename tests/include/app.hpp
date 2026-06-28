#pragma once
#ifdef ATOM_EXECUTION
    #undef ATOM_EXECUTION
#endif
#include <cstddef>
#include <memory_resource>
#include <tuple>
#include <neutron/ecs.hpp>
#include <neutron/execution.hpp>
#include <neutron/metafn.hpp>
#include "thread_pool.hpp"

using commands = neutron::basic_commands<std::pmr::polymorphic_allocator<>>;

class myapp {
    struct single_frame {
        int polls = 0;

        bool poll_events() noexcept {
            ++polls;
            return true;
        }

        [[nodiscard]] bool is_stopped() const noexcept { return polls > 1; }
    };

public:
    using config_type = std::tuple<>;
    static myapp create() { return {}; }
    template <auto... Worlds>
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

        if constexpr (sizeof...(Worlds) != 0) {
            auto hooks = single_frame{};
            auto rt    = make_runtime<Worlds...>(sch, &hooks, alloc);
            rt.run();
        }
    }
};
