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

using namespace neutron;
using enum stage;

constexpr std::uint8_t total_ticks = 8;

struct payload {
    std::uint8_t ticks = 0;
    bool poll_events() noexcept {
        ++ticks;
        return false;
    }

    [[nodiscard]] bool is_running() const noexcept {
        return ticks < total_ticks;
    }

    void render_begin() {}

    void render_end() {}
};

void test_make_runtime() {
    // thread_pool pool;
    // payload payload;
    // make_runtime<world_desc>(pool, &payload);
}

int main() {
    // test_make_runtime();

    return 0;
}
