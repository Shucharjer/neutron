#include "neutron/detail/ecs/runtime.hpp"
#include <type_traits>
#include <utility>
#include <neutron/ecs.hpp>
#include "thread_pool.hpp"

using namespace neutron;
using enum stage;

struct marker {
    int value = 0;
};

template <>
constexpr bool neutron::as_component<marker> = true;

struct impl {
    int updates = 0;

    bool poll_events() { return false; }
    bool is_stopped() { return updates++ != 0; }
    void render_begin() {}
    void render_end() {}
};

class app : impl {
    app() = default;

public:
    static app create() { return {}; }

    template <auto... Worlds>
    int run() {
        using namespace thread_pool_for_test;
        thread_pool pool;

        scheduler auto sch = pool.get_scheduler();
        auto rt = make_runtime<Worlds...>(sch, static_cast<impl*>(this));
        return rt.run();
    }
};

void foo() {}
void bar() {}
int marker_count = 0;

void spawn_marker(commands cmd) { cmd.spawn<marker>(); }

void count_marker(query<with<marker&>> query) {
    marker_count = 0;
    for ([[maybe_unused]] auto marker : query.get()) {
        ++marker_count;
    }
}

constexpr auto world1 = world_desc | add_systems<update, foo>;
constexpr auto world2 =
    world_desc | add_systems<update, foo> | execute<group<2>>;
constexpr auto world3 =
    world_desc | add_systems<update, foo> | execute<individual>;
constexpr auto world4 =
    world_desc | add_systems<update, foo> | execute<group<2>>;
constexpr auto world5 =
    world_desc | add_systems<update, foo> | execute<individual>;

constexpr auto task_world =
    world_desc | add_systems<update, foo, { bar, after<foo> }>;
constexpr auto command_world =
    world_desc |
    add_systems<update, spawn_marker, { count_marker, after<spawn_marker> }>;
constexpr auto group_world_a =
    world_desc | add_systems<update, foo> | execute<group<3>>;
constexpr auto group_world_b =
    world_desc | add_systems<update, foo> | execute<group<3>>;
constexpr auto group_world_c =
    world_desc | add_systems<update, foo> | execute<group<3>>;

template <typename Runtime>
using first_run_env_t = std::remove_cvref_t<
    decltype(std::declval<Runtime&>().template run_env<0>())>;

using command_world_state = neutron::_runtime::_world_state<
    std::remove_cvref_t<decltype(command_world)>, std::allocator<std::byte>>;
static_assert(!noexcept(std::declval<command_world_state&>().flush(0)));

int main() {
    thread_pool pool;
    auto sch = pool.get_scheduler();
    auto rt  = make_runtime<world1, world2, world3, world4, world5>(
        sch, static_cast<impl*>(nullptr));

    auto typed_world = make_world<task_world>();
    constexpr auto tasks =
        std::remove_cvref_t<decltype(typed_world)>::get_tasks();
    using update_tasks = typename decltype(tasks)::template graph<update>;
    static_assert(update_tasks::size == 2);
    static_assert(update_tasks::predecessor_counts[1] == 1);

    auto task_rt = make_runtime<task_world>(sch, static_cast<impl*>(nullptr));
    static_assert(decltype(task_rt)::run_env_count == 1);
    static_assert(
        requires { task_rt.template run_env<0>().template world<0>(); });
    using single_group_env = first_run_env_t<decltype(task_rt)>;
    static_assert(
        single_group_env::max_concurrency == get_max_concurrency(task_world));

    auto group_rt = make_runtime<group_world_a, group_world_b, group_world_c>(
        sch, static_cast<impl*>(nullptr));
    using multi_group_env = first_run_env_t<decltype(group_rt)>;
    static_assert(
        multi_group_env::max_concurrency == get_max_concurrency(group_world_a));

    impl command_payload{};
    auto command_rt = make_runtime<command_world>(sch, &command_payload);
    marker_count    = 0;
    command_rt.run();
    if (marker_count != 1) {
        return 1;
    }

    return app::create() | run_worlds<world1, world2, world3, world4, world5>();
}
