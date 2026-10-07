#include <atomic>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <neutron/detail/ecs/runtime/runtime.hpp>
#include <neutron/ecs.hpp>
#include <neutron/execution.hpp>
#include "neutron/detail/reflection/refl.hpp"
#include "neutron/print.hpp"
#include "thread_pool.hpp"

using namespace std::chrono_literals;
using namespace neutron;
using enum stage;

static_assert(scheduler_provider<thread_pool>);
static_assert(parallelism_scheduler_provider<thread_pool>);

using alloc_t = std::allocator<std::byte>;

using time_point = std::chrono::system_clock::time_point;

struct payload {
    time_point tp;

    payload(std::chrono::seconds seconds) noexcept
        : tp(std::chrono::system_clock::now() + seconds) {}

    bool poll_events() noexcept { return false; }

    [[nodiscard]] bool is_running() const noexcept {
        auto now = std::chrono::system_clock::now();
        return now < tp;
    }

    void render_begin() {}

    void render_end() {}
};

void tick(global<std::uint32_t> glob) {
    auto& [count] = glob;
    ++count;
}

void println_ticks(global<std::uint32_t> glob) {
    auto& [count] = glob;
    println("ticks: {}", count);
}

constexpr auto main_world = world_desc | add_systems<preupdate, &tick> |
                            add_systems<shutdown, &println_ticks>;

void test_runtime() {
    thread_pool pool;
    payload payload(3s);
    auto rt = make_runtime<main_world>(pool, &payload);
    rt.run();
}

void test_individual() {
    constexpr auto idvj_world = world_desc | add_systems<preupdate, &tick> |
                                add_systems<shutdown, &println_ticks> |
                                execute<individual, tick_rate<128>>;

    thread_pool pool;
    payload payload(3s);
    auto rt = make_runtime<main_world, idvj_world>(pool, &payload);
    rt.run();
}

void test_same_group() {
    constexpr auto same_group = world_desc | add_systems<preupdate, &tick> |
                                add_systems<shutdown, &println_ticks> |
                                execute<tick_rate<128>>;

    thread_pool pool;
    payload payload(3s);
    auto rt = make_runtime<main_world, same_group>(pool, &payload);
    rt.run();
}

void test_another_group() {
    constexpr auto another_group = world_desc | add_systems<preupdate, &tick> |
                                   add_systems<shutdown, &println_ticks> |
                                   execute<group<1>, tick_rate<128>>;

    thread_pool pool;
    payload payload(3s);
    auto rt = make_runtime<main_world, another_group>(pool, &payload);
    rt.run();
}

int main() {
    test_runtime();
    test_individual();
    test_same_group();
    test_another_group();

    return 0;
}
