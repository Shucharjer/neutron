#ifdef NDEBUG
    #undef NDEBUG
#endif
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>
#include <neutron/ecs.hpp>
#include <neutron/metafn.hpp>
#include "neutron/print.hpp"
#include "thread_pool.hpp"

using namespace neutron;
using enum stage;

static_assert(!is_hifreq_stage(startup));
static_assert(!is_hifreq_stage(first));
static_assert(!is_hifreq_stage(shutdown));
static_assert(is_hifreq_stage(events));
static_assert(is_hifreq_stage(render));

using alloc_t = std::allocator<std::byte>;

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

void test_individual_env() {
    constexpr auto idvj_world = world_desc | add_systems<preupdate, &tick> |
                                add_systems<shutdown, &println_ticks> |
                                execute<individual, tick_rate<128>>;

    using run_envs = run_envs_for<alloc_t, main_world, idvj_world>;
    static_assert(std::tuple_size_v<run_envs> == 2);
    static_assert(run_env<std::tuple_element_t<0, run_envs>>);
    static_assert(run_env<std::tuple_element_t<1, run_envs>>);
}

int executions = 0;

void count_execution() { ++executions; }

void test_lazy_sender() {
    constexpr auto desc = world_desc | add_systems<update, &count_execution>;
    run_envs_for<alloc_t, desc> envs;
    auto sender =
        envs.run<update>(execution::inline_scheduler{}, _time_point_t{});
    static_assert(execution::sender<decltype(sender)>);
    assert(executions == 0);
    auto result = this_thread::sync_wait(std::move(sender));
    assert(result.has_value());
    assert(executions == 1);
}

std::vector<int> order;
void root_task() { order.push_back(1); }
void middle_task() { order.push_back(2); }
void leaf_task() { order.push_back(3); }

void test_dependencies() {
    constexpr auto desc =
        world_desc | add_systems<
                         update, { &leaf_task, after<&middle_task> },
                         { &root_task, before<&middle_task, &middle_task> },
                         { &middle_task, after<&root_task> }>;
    constexpr auto graph =
        graph_builder<decltype(get_systems<update>(desc))>::make();
    static_assert(graph[0].size() == 1 && graph[0][0] == 2);
    static_assert(graph[2].size() == 1 && graph[2][0] == 1);
    run_envs_for<alloc_t, desc> envs;
    thread_pool pool(2);
    this_thread::sync_wait(envs.run<update>(pool.get_scheduler(), {}));
    assert((order == std::vector{ 1, 2, 3 }));
}

int fast_count    = 0;
int slow_count    = 0;
int pre_count     = 0;
int startup_count = 0;
void fast_task() { ++fast_count; }
void slow_task() { ++slow_count; }
void pre_task() { ++pre_count; }
void startup_task() { ++startup_count; }

void test_tick_rates() {
    using namespace std::chrono_literals;
    constexpr auto desc = world_desc | execute<tick_rate<10>> |
                          add_systems<startup, &startup_task> |
                          add_systems<preupdate, &pre_task> |
                          add_systems<
                              update, { &fast_task, after<&slow_task> },
                              { &slow_task, tick_rate<2> }>;
    run_envs_for<alloc_t, desc> envs;
    auto scheduler                  = execution::inline_scheduler{};
    auto time                       = _time_point_t{};
    [[maybe_unused]] auto abandoned = envs.run<update>(scheduler, time);
    assert(fast_count == 0 && slow_count == 0);
    this_thread::sync_wait(envs.run<preupdate>(scheduler, time));
    this_thread::sync_wait(envs.run<update>(scheduler, time));
    assert(pre_count == 1 && fast_count == 1 && slow_count == 1);
    this_thread::sync_wait(envs.run<update>(scheduler, time + 99ms));
    assert(fast_count == 1 && slow_count == 1);
    this_thread::sync_wait(envs.run<update>(scheduler, time + 100ms));
    assert(fast_count == 2 && slow_count == 1);
    this_thread::sync_wait(envs.run<update>(scheduler, time + 500ms));
    assert(fast_count == 3 && slow_count == 2);
    this_thread::sync_wait(envs.run<startup>(scheduler, time));
    this_thread::sync_wait(envs.run<startup>(scheduler, time));
    assert(startup_count == 2);
}

int local_count = 0;
void local_task(local<int> state) {
    auto& [count] = state;
    local_count   = ++count;
}

void test_parameters() {
    constexpr auto desc = world_desc | add_systems<update, &local_task, &tick>;
    run_envs_for<alloc_t, desc> envs;
    auto scheduler = execution::inline_scheduler{};
    this_thread::sync_wait(envs.run<update>(scheduler, {}));
    this_thread::sync_wait(envs.run<update>(scheduler, {}));
    assert(local_count == 2);
    assert(std::get<0>(global<std::uint32_t>{}) == 2);
}

struct test_resource {
    using resource_concept = resource_tag;
    int value              = 0;
};

int parameter_result = 0;

int startup_parameters(
    local<int> state, res<test_resource> resources) noexcept {
    auto& [count]    = state;
    auto& [resource] = resources;
    parameter_result = ++count + ++resource.value;
    return parameter_result;
}

static_assert(_task_spec_matches<
              &startup_parameters,
              static_cast<int (*)(local<int>, res<test_resource>)>(
                  &startup_parameters)>());

void read_resource(res<const test_resource> resources) {
    auto& [resource] = resources;
    assert(resource.value == 2);
}

void test_startup_parameters() {
    constexpr auto desc = world_desc |
                          add_systems<startup, &startup_parameters> |
                          add_systems<shutdown, &read_resource>;
    auto world     = make_world<desc>();
    auto scheduler = execution::inline_scheduler{};
    this_thread::sync_wait(world.run<startup>(scheduler, {}));
    this_thread::sync_wait(world.run<startup>(scheduler, {}));
    assert(parameter_result == 4);
    this_thread::sync_wait(world.run<shutdown>(scheduler, {}));
}

void test_empty_and_multiple_worlds() {
    run_envs_for<alloc_t> empty;
    auto scheduler = execution::inline_scheduler{};
    assert(this_thread::sync_wait(empty.run<update>(scheduler, {})));
    constexpr auto first  = world_desc | add_systems<update, &count_execution>;
    constexpr auto second = first | set_identifier<"second">;
    constexpr auto third  = first | execute<individual>;
    run_envs_for<alloc_t, first, second, third> envs;
    auto previous = executions;
    this_thread::sync_wait(envs.run<update>(scheduler, {}));
    assert(executions == previous + 3);
    this_thread::sync_wait(envs.run<shutdown>(scheduler, {}));
    assert(executions == previous + 3);
}

std::atomic<int> arrivals    = 0;
std::atomic<int> completions = 0;

template <int Index>
void parallel_task() {
    using namespace std::chrono_literals;
    ++arrivals;
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (arrivals.load() != 2 &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    assert(arrivals == 2);
    ++completions;
}

void join_task() { assert(completions == 2); }

void test_individual_dependency_join() {
    constexpr auto desc =
        world_desc |
        add_systems<
            update, &parallel_task<0>,
            { &parallel_task<1>, individual, tick_rate<1> },
            { &join_task, after<&parallel_task<0>, &parallel_task<1>> }>;
    run_envs_for<alloc_t, desc> envs;
    thread_pool pool(2);
    assert(this_thread::sync_wait(envs.run<update>(pool.get_scheduler(), {})));
    assert(completions == 2);
}

void test_dynamic_tick_rates() {
    using namespace std::chrono_literals;
    constexpr auto desc =
        world_desc | execute<dynamic_tick_rate> |
        add_systems<
            update, { &count_execution, dynamic_tick_rate, individual }>;
    run_envs_for<alloc_t, desc> envs;
    auto& world = std::get<0>(envs).get<0>();
    world.set_tick_rate(10);
    world.set_system_tick_rate<update, &count_execution>(2);
    assert(world.get_tick_rate() == 10);
    auto scheduler = execution::inline_scheduler{};
    auto time      = _time_point_t{};
    auto previous  = executions;
    this_thread::sync_wait(envs.run<update>(scheduler, time));
    this_thread::sync_wait(envs.run<update>(scheduler, time + 100ms));
    assert(executions == previous + 1);
    this_thread::sync_wait(envs.run<update>(scheduler, time + 500ms));
    assert(executions == previous + 2);
    world.set_tick_rate(0);
    world.set_system_tick_rate<update, &count_execution>(0);
    this_thread::sync_wait(envs.run<update>(scheduler, time + 501ms));
    assert(executions == previous + 3);
}

int error_successors = 0;
void throw_task() { throw std::runtime_error("task failure"); }
void error_successor() { ++error_successors; }

void test_error_propagation() {
    constexpr auto desc =
        world_desc | execute<tick_rate<1>> |
        add_systems<
            update, &throw_task, { &error_successor, after<&throw_task> }>;
    run_envs_for<alloc_t, desc> envs;
    bool received = false;
    auto sender   = envs.run<update>(execution::inline_scheduler{}, {}) |
                  execution::upon_error([&received](std::exception_ptr error) {
                      assert(error);
                      received = true;
                  });
    assert(this_thread::sync_wait(std::move(sender)));
    assert(received && error_successors == 0);
}

void test_stopped_propagation() {
    constexpr auto desc = world_desc | execute<tick_rate<1>> |
                          add_systems<update, &count_execution>;
    run_envs_for<alloc_t, desc> envs;
    thread_pool pool(1);
    execution::inplace_stop_source source;
    source.request_stop();
    auto previous = executions;
    auto sender   = execution::write_env(
        envs.run<update>(pool.get_scheduler(), {}),
        execution::prop{ execution::get_stop_token, source.get_token() });
    assert(!this_thread::sync_wait(std::move(sender)));
    assert(executions == previous);
    this_thread::sync_wait(envs.run<update>(pool.get_scheduler(), {}));
    assert(executions == previous + 1);
}

int main() {
    test_individual_env();
    test_lazy_sender();
    test_dependencies();
    test_tick_rates();
    test_parameters();
    test_startup_parameters();
    test_empty_and_multiple_worlds();
    test_individual_dependency_join();
    test_dynamic_tick_rates();
    test_error_propagation();
    test_stopped_propagation();

    return 0;
}
