#include <stdexcept>
#include <thread>
#include <tuple>
#include <utility>
#include <neutron/execution.hpp>

namespace ex = neutron::execution;

namespace {

static_assert(ex::sender<decltype(ex::just(1))>);
static_assert(ex::sender<decltype(ex::just())>);

bool test_just_then() {
    auto sndr    = ex::just(40) | ex::then([](int value) {
                    return value + 2;
                });
    auto [value] = neutron::this_thread::sync_wait(std::move(sndr)).value();
    return value == 42;
}

bool test_multiple_values() {
    auto result =
        neutron::this_thread::sync_wait(ex::just(1, 'a', 2.5)).value();
    return result == std::tuple{ 1, 'a', 2.5 };
}

bool test_let_value() {
    auto sndr    = ex::just(20) | ex::let_value([](int value) {
                    return ex::just(value * 2);
                });
    auto [value] = neutron::this_thread::sync_wait(std::move(sndr)).value();
    return value == 40;
}

bool test_when_all() {
    auto sndr      = ex::when_all(ex::just(1), ex::just(2, 3));
    auto [a, b, c] = neutron::this_thread::sync_wait(std::move(sndr)).value();
    return a == 1 && b == 2 && c == 3;
}

bool test_upon_error() {
    auto sndr    = ex::just_error(7) | ex::upon_error([](int error) {
                    return error * 6;
                });
    auto [value] = neutron::this_thread::sync_wait(std::move(sndr)).value();
    return value == 42;
}

bool test_upon_stopped() {
    auto sndr    = ex::just_stopped() | ex::upon_stopped([] {
                    return 42;
                });
    auto [value] = neutron::this_thread::sync_wait(std::move(sndr)).value();
    return value == 42;
}

bool test_unstoppable_by_default() {
    auto [token] =
        neutron::this_thread::sync_wait(ex::read_env(neutron::get_stop_token))
            .value();
    return !token.stop_possible() && !token.stop_requested();
}

bool test_run_loop() {
    ex::run_loop loop;
    auto sched = loop.get_scheduler();
    static_assert(ex::scheduler<decltype(sched)>);

    std::thread::id worker_id;
    std::jthread worker([&] {
        worker_id = std::this_thread::get_id();
        loop.run();
    });

    auto sndr     = ex::schedule(sched) | ex::then([] {
                    return std::this_thread::get_id();
                });
    auto [ran_on] = neutron::this_thread::sync_wait(std::move(sndr)).value();
    loop.finish();
    worker.join();
    return ran_on == worker_id;
}

} // namespace

int main() {
    if (!test_just_then()) {
        return 1;
    }
    if (!test_multiple_values()) {
        return 2;
    }
    if (!test_let_value()) {
        return 3;
    }
    if (!test_when_all()) {
        return 4;
    }
    if (!test_upon_error()) {
        return 5;
    }
    if (!test_upon_stopped()) {
        return 6;
    }
    if (!test_unstoppable_by_default()) {
        return 7;
    }
    if (!test_run_loop()) {
        return 8;
    }
    return 0;
}
