#include <stdexcept>
#include <thread>
#include <tuple>
#include <utility>
#include <neutron/execution.hpp>
#include "require.hpp"

namespace ex = neutron::execution;

namespace {

static_assert(ex::sender<decltype(ex::just(1))>);
static_assert(ex::sender<decltype(ex::just())>);

void test_then_transforms_the_value() {
    auto sndr = ex::just(40) | ex::then([](int value) {
                    return value + 2;
                });

    auto result = neutron::this_thread::sync_wait(std::move(sndr));

    require_or_return(result.has_value(), );
    auto [value] = *result;
    require(value == 42);
}

void test_just_sends_multiple_values() {
    auto result = neutron::this_thread::sync_wait(ex::just(1, 'a', 2.5));

    require_or_return(result.has_value(), );
    require(*result == std::tuple{ 1, 'a', 2.5 });
}

void test_let_value_continues_with_a_new_sender() {
    auto sndr = ex::just(20) | ex::let_value([](int value) {
                    return ex::just(value * 2);
                });

    auto result = neutron::this_thread::sync_wait(std::move(sndr));

    require_or_return(result.has_value(), );
    auto [value] = *result;
    require(value == 40);
}

void test_when_all_concatenates_the_values() {
    auto sndr = ex::when_all(ex::just(1), ex::just(2, 3));

    auto result = neutron::this_thread::sync_wait(std::move(sndr));

    require_or_return(result.has_value(), );
    auto [a, b, c] = *result;
    require(a == 1);
    require(b == 2);
    require(c == 3);
}

void test_upon_error_recovers_from_an_error() {
    auto sndr = ex::just_error(7) | ex::upon_error([](int error) {
                    return error * 6;
                });

    auto result = neutron::this_thread::sync_wait(std::move(sndr));

    require_or_return(result.has_value(), );
    auto [value] = *result;
    require(value == 42);
}

void test_upon_stopped_recovers_from_a_stop() {
    auto sndr = ex::just_stopped() | ex::upon_stopped([] {
                    return 42;
                });

    auto result = neutron::this_thread::sync_wait(std::move(sndr));

    require_or_return(result.has_value(), );
    auto [value] = *result;
    require(value == 42);
}

void test_sync_wait_stop_token_is_unstoppable() {
    auto result =
        neutron::this_thread::sync_wait(ex::read_env(neutron::get_stop_token));

    require_or_return(result.has_value(), );
    auto [token] = *result;
    require_false(token.stop_possible());
    require_false(token.stop_requested());
}

void test_run_loop_runs_work_on_its_thread() {
    ex::run_loop loop;
    auto sched = loop.get_scheduler();
    static_assert(ex::scheduler<decltype(sched)>);
    std::thread::id worker_id;
    std::jthread worker([&] {
        worker_id = std::this_thread::get_id();
        loop.run();
    });

    auto sndr   = ex::schedule(sched) | ex::then([] {
                    return std::this_thread::get_id();
                });
    auto result = neutron::this_thread::sync_wait(std::move(sndr));
    // Stop the worker before any check may return early.
    loop.finish();
    worker.join();

    require_or_return(result.has_value(), );
    auto [ran_on] = *result;
    require(ran_on == worker_id);
}

} // namespace

int main() {
    test_then_transforms_the_value();
    test_just_sends_multiple_values();
    test_let_value_continues_with_a_new_sender();
    test_when_all_concatenates_the_values();
    test_upon_error_recovers_from_an_error();
    test_upon_stopped_recovers_from_a_stop();
    test_sync_wait_stop_token_is_unstoppable();
    test_run_loop_runs_work_on_its_thread();
    return neutron_test::result();
}
