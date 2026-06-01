#include <atomic>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <neutron/detail/ecs/runtime.hpp>
#include <neutron/ecs.hpp>
#include <neutron/execution.hpp>
#include "neutron/print.hpp"
#include "thread_pool.hpp"

using namespace neutron;
using enum stage;

constexpr std::uint8_t total_ticks = 8;

template <typename SchedulerProvider>
class app {
    app(SchedulerProvider& sp) : sp_(sp) {}

    struct payload {
        std::uint8_t ticks = 0;

        bool poll_events() { ++ticks; }

        [[nodiscard]] bool is_running() const noexcept {
            return ticks < total_ticks;
        }

        void render_begin() {}

        void render_end() {}
    };

public:
    template <auto... Worlds>
    int run() {
        payload payload;
        auto rt = make_runtime<Worlds...>(sp_, &payload);

        return rt.run();
    }

    static app create(SchedulerProvider& sp) { return app(sp); }

private:
    SchedulerProvider& sp_; // NOLINT
};

template <>
constexpr bool as_resource<std::atomic<int>> = true;

void fetch_add(global<std::atomic<int>&> res) {
    auto& [atomic] = res;
    atomic.fetch_add(1);
}

constexpr auto add_fetch_add = add_systems<update, fetch_add>;

void requires_same(global<std::atomic<int>&> res) {
    auto& [atomic] = res;
    auto result    = atomic.load();
    if (result != total_ticks) {
        std::string errinfo =
            "the result of fetch_add not equals to expected (" +
            std::to_string(total_ticks) +
            "), result: " + std::to_string(result);
        throw std::logic_error(errinfo);
    }
}

constexpr auto add_requires_same = add_systems<shutdown, requires_same>;

void may_not_same(global<std::atomic<int>&> res) {
    auto& [atomic] = res;
    auto result    = atomic.load();
    println("fetch add result: {}", result);
}

constexpr auto add_may_not_same = add_systems<shutdown, may_not_same>;

template <typename SchedulerProvider>
void test_run_one(SchedulerProvider& sp) {
    constexpr auto desc = world_desc | add_fetch_add | add_requires_same;

    app<SchedulerProvider>::create(sp) | run_worlds<desc>();
}

template <typename SchedulerProvider>
void test_run_two_in_same_group(SchedulerProvider& sp) {
    constexpr auto desc1 = world_desc | add_requires_same;
    constexpr auto desc2 = world_desc | add_fetch_add;

    app<SchedulerProvider>::create(sp) | run_worlds<desc1, desc2>();
}

template <typename SchedulerProvider>
void test_run_two_individual(SchedulerProvider& sp) {
    constexpr auto desc1 = world_desc | add_requires_same;
    constexpr auto desc2 = world_desc | add_fetch_add | execute<individual>;

    app<SchedulerProvider>::create(sp) | run_worlds<desc1, desc2>();
}

int main() {

    {
        thread_pool pool;
        test_run_one(pool);
        test_run_two_in_same_group(pool);
        test_run_two_individual(pool);
    }

    {
        execution::run_loop loop;
        test_run_one(loop);
        test_run_two_in_same_group(loop);
        test_run_two_individual(loop);
    }

    return 0;
}
