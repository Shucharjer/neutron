#include <atomic>
#include <chrono>
#include <thread>
#include <neutron/detail/ecs/runtime.hpp>
#include <neutron/detail/ecs/world_descriptor.hpp>
#include <neutron/ecs.hpp>
#include <neutron/execution.hpp> // IWYU pragma: keep, for run_loop
#include "require.hpp"
#include "thread_pool.hpp"

using namespace neutron;
using enum stage;

std::atomic<int> flat_started = 0;
std::atomic<int> flat_passed  = 0;

void flat_task() {
    flat_started.fetch_add(1, std::memory_order_acq_rel);
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds{ 1 };
    while (flat_started.load(std::memory_order_acquire) < 2 &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    if (flat_started.load(std::memory_order_acquire) >= 2) {
        flat_passed.fetch_add(1, std::memory_order_acq_rel);
    }
}

struct impl {
    int updates = 0;

    bool poll_events() noexcept {
        ++updates;
        return false;
    }
    [[nodiscard]] bool is_stopped() const noexcept { return updates < 8; }
    void render_begin() {}
    void render_end() {}
};

int do_test(auto& sch) {
    constexpr void (*fn1)() = [] {};
    constexpr void (*fn2)() = [] {};

    constexpr auto world1 =
        world_desc | enable_events | enable_render | add_systems<update, fn1>;
    constexpr auto world2 = world_desc | add_systems<update, fn2>;

    impl impl;
    auto rt = make_runtime<world1, world2>(sch, &impl);
    return rt.run();
}

int test_with_run_loop() {
    execution::run_loop loop;
    auto sch = loop.get_scheduler();
    return do_test(sch);
}

int test_with_pool() {
    thread_pool pool;
    auto sch = pool.get_scheduler();
    return do_test(sch);
}

int test_flat_group_dispatch() {
    constexpr auto world1 = world_desc | add_systems<update, &flat_task>;
    constexpr auto world2 = world_desc | add_systems<update, &flat_task>;

    struct single_frame {
        int polls = 0;

        bool poll_events() noexcept {
            ++polls;
            return true;
        }

        [[nodiscard]] bool is_stopped() const noexcept { return polls > 1; }
    };

    flat_started.store(0, std::memory_order_release);
    flat_passed.store(0, std::memory_order_release);

    single_frame hooks;
    thread_pool pool(3);
    auto sch = pool.get_scheduler();
    auto rt  = make_runtime<world1, world2>(sch, &hooks);
    rt.run();
    return flat_passed.load(std::memory_order_acquire) == 2 ? 0 : 1;
}

int main() {
    require_or_return(test_with_pool() == 0, 1);
    require_or_return(test_with_pool() == 0, 2);
    require_or_return(test_flat_group_dispatch() == 0, 3);
    return 0;
}
