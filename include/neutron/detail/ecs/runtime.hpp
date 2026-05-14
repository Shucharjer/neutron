#pragma once
#include <array>
#include <condition_variable>
#include <csignal>
#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "neutron/detail/ecs/world.hpp"
#include "neutron/detail/ecs/world_accessor.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_events.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_execution_interval.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_execution_policy.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_max_buffer_count.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_max_concurrency.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_min_concurrency.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_render.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/metafn/cat.hpp"
#include "neutron/detail/tuple/shared_tuple.hpp"
#include "neutron/execution.hpp"

namespace neutron {

template <std::size_t GroupID, typename Alloc, auto... Worlds>
class run_env_for_group;

template <typename Alloc, auto World>
class run_env_for_individual;

template <auto World>
struct _run_env_for_individual_spec {};

template <typename, auto... Worlds>
struct _to_run_envs;
template <typename Curr, auto World>
struct _to_run_envs<Curr, World> {
    using policy = std::remove_const_t<decltype(get_execution_policy(World))>;
    using type   = decltype([] {
        if constexpr (std::same_as<policy, _individual_t>) {
            return type_list_cat_t<
                  Curr, type_list<_run_env_for_individual_spec<World>>>{};
        } else {
            return insert_tagged_value_list_inplace_t<policy, Curr, World>{};
        }
    }());
};
template <typename Curr, auto World, auto... Others>
struct _to_run_envs<Curr, World, Others...> {
    using policy = std::remove_const_t<decltype(get_execution_policy(World))>;
    using result = decltype([] {
        if constexpr (std::same_as<policy, _individual_t>) {
            return type_list_cat_t<
                Curr, type_list<_run_env_for_individual_spec<World>>>{};
        } else {
            return insert_tagged_value_list_inplace_t<policy, Curr, World>{};
        }
    }());
    using type   = typename _to_run_envs<result, Others...>::type;
};
template <auto... Worlds>
using _to_run_envs_t = typename _to_run_envs<type_list<>, Worlds...>::type;

template <typename Alloc, typename>
struct _make_run_env_impl;
template <typename Alloc, std::size_t GroupId, auto... Worlds>
struct _make_run_env_impl<
    Alloc, tagged_value_list<_group_t<GroupId>, Worlds...>> {
    using type = run_env_for_group<GroupId, Alloc, Worlds...>;
};
template <typename Alloc, auto World>
struct _make_run_env_impl<Alloc, _run_env_for_individual_spec<World>> {
    using type = run_env_for_individual<Alloc, World>;
};

template <typename Alloc, typename>
struct _make_run_envs;
template <
    typename Alloc, template <typename...> typename Template, typename... Tys>
struct _make_run_envs<Alloc, Template<Tys...>> {
    using type = Template<typename _make_run_env_impl<Alloc, Tys>::type...>;
};
template <typename Alloc, typename Tys>
using _make_run_envs_t = typename _make_run_envs<Alloc, Tys>::type;

namespace _runtime {

template <typename RunEnvs>
struct _run_env_pack_traits;

template <template <typename...> typename Template, typename... Envs>
struct _run_env_pack_traits<Template<Envs...>> {
    static constexpr std::size_t render_enabled_count =
        (std::size_t{ 0 } + ... + Envs::render_enabled_count);
    static constexpr std::size_t events_enabled_count =
        (std::size_t{ 0 } + ... + Envs::events_enabled_count);
};

template <typename Ty, typename... Tys>
consteval auto _common_max(Ty value, Tys... values) noexcept {
    using result_type = std::common_type_t<Ty, Tys...>;
    auto result       = static_cast<result_type>(value);
    ((result = result < static_cast<result_type>(values)
                   ? static_cast<result_type>(values)
                   : result),
     ...);
    return result;
}

template <typename Payload, typename Runtime>
concept _poll_events_with_runtime =
    requires(Payload& payload, Runtime& runtime) {
        { payload.poll_events(runtime) } -> std::convertible_to<bool>;
    };

template <typename Payload>
concept _poll_events_plain = requires(Payload& payload) {
    { payload.poll_events() } -> std::convertible_to<bool>;
};

template <typename Payload, typename Runtime>
concept _is_stopped_with_runtime =
    requires(Payload& payload, Runtime& runtime) {
        { payload.is_stopped(runtime) } -> std::convertible_to<bool>;
    };

template <typename Payload>
concept _is_stopped_plain = requires(Payload& payload) {
    { payload.is_stopped() } -> std::convertible_to<bool>;
};

template <typename Payload, typename Runtime>
concept _render_begin_with_runtime = requires(
    Payload& payload, Runtime& runtime) { payload.render_begin(runtime); };

template <typename Payload>
concept _render_begin_plain =
    requires(Payload& payload) { payload.render_begin(); };

template <typename Payload, typename Runtime>
concept _render_end_with_runtime = requires(
    Payload& payload, Runtime& runtime) { payload.render_end(runtime); };

template <typename Payload>
concept _render_end_plain =
    requires(Payload& payload) { payload.render_end(); };

template <std::size_t Size>
class _completion_queue {
public:
    struct record {
        std::size_t index{};
        std::exception_ptr error{};
        bool stopped = false;
    };

    void push(record item) noexcept {
        {
            std::lock_guard lock{ mutex_ };
            records_[write_] = std::move(item);
            write_           = (write_ + 1) % Size;
            ++count_;
        }
        cv_.notify_one();
    }

    [[nodiscard]] auto wait_pop() -> record {
        std::unique_lock lock{ mutex_ };
        cv_.wait(lock, [this] { return count_ != 0; });
        return _pop_locked();
    }

    [[nodiscard]] auto try_pop(record& item) -> bool {
        std::lock_guard lock{ mutex_ };
        if (count_ == 0) {
            return false;
        }
        item = _pop_locked();
        return true;
    }

private:
    [[nodiscard]] auto _pop_locked() -> record {
        auto item = std::move(records_[read_]);
        read_     = (read_ + 1) % Size;
        --count_;
        return item;
    }

    std::array<record, Size> records_{};
    std::mutex mutex_{};
    std::condition_variable cv_{};
    std::size_t read_  = 0;
    std::size_t write_ = 0;
    std::size_t count_ = 0;
};

template <std::size_t Size>
struct _completion_receiver {
    using receiver_concept = execution::receiver_tag;

    _completion_queue<Size>* queue;
    std::size_t index;

    void set_value() && noexcept { queue->push({ .index = index }); }

    template <typename Error>
    void set_error(Error&& error) && noexcept {
        if constexpr (std::same_as<
                          std::remove_cvref_t<Error>, std::exception_ptr>) {
            queue->push(
                { .index = index, .error = std::forward<Error>(error) });
        } else {
            queue->push(
                { .index = index,
                  .error =
                      std::make_exception_ptr(std::forward<Error>(error)) });
        }
    }

    void set_stopped() && noexcept {
        queue->push({ .index = index, .stopped = true });
    }

    [[nodiscard]] auto get_env() const noexcept { return execution::env<>{}; }
};

template <stage Stage, typename World>
struct _task_fn {
    using command_buffer = typename World::command_buffer;

    World* world;
    std::size_t index;
    command_buffer* cmdbuf;

    void operator()() const {
        world_accessor::call_task<Stage>(*world, index, cmdbuf);
    }
};

template <typename Sender, std::size_t Size>
struct _connected_task {
    using opstate_type = decltype(execution::connect(
        std::declval<Sender>(), std::declval<_completion_receiver<Size>>()));

    template <typename Sndr>
    _connected_task(
        Sndr&& sender, _completion_queue<Size>& queue, std::size_t index)
        : opstate(
              execution::connect(
                  std::forward<Sndr>(sender),
                  _completion_receiver<Size>{ &queue, index })) {}

    void start() noexcept { execution::start(opstate); }

    opstate_type opstate;
};

template <typename Descriptor, typename Alloc>
class _world_state {
    using byte_alloc = rebind_alloc_t<Alloc, std::byte>;

public:
    using descriptor_type = Descriptor;
    using world_type      = basic_world<Descriptor, byte_alloc>;
    using command_buffer  = typename world_type::command_buffer;

private:
    template <typename Ty>
    using allocator_t = rebind_alloc_t<byte_alloc, Ty>;
    using buffer_vector =
        std::vector<command_buffer, allocator_t<command_buffer>>;

public:
    explicit _world_state(const Alloc& alloc = {})
        : alloc_(alloc), world_(byte_alloc{ alloc }),
          buffers_(allocator_t<command_buffer>{ alloc }) {
        if constexpr (max_buffer_count != 0) {
            buffers_.reserve(max_buffer_count);
            for (std::size_t index = 0; index < max_buffer_count; ++index) {
                buffers_.emplace_back(byte_alloc{ alloc });
            }
        }
    }

    [[nodiscard]] auto world() noexcept -> world_type& { return world_; }

    [[nodiscard]] auto world() const noexcept -> const world_type& {
        return world_;
    }

    template <stage Stage, execution::scheduler Sch>
    void run_stage(Sch& sch) {
        using graph =
            typename decltype(world_type::get_tasks())::template graph<Stage>;
        _run_stage<Stage, graph>(sch);
    }

    template <stage Stage>
    void step_stage() {
        using graph =
            typename decltype(world_type::get_tasks())::template graph<Stage>;
        _step_stage<Stage, graph>();
    }

    void flush(std::size_t count) {
        for (std::size_t index = 0; index < count; ++index) {
            buffers_[index].apply(world_accessor::base(world_));
            buffers_[index].reset();
        }
    }

    static constexpr std::uint32_t max_buffer_count =
        get_max_buffer_count(Descriptor{});

private:
    template <typename Graph>
    [[nodiscard]] static std::size_t _collect_runnable(
        const stage_task_graph<Graph>& graph,
        const std::array<bool, Graph::size>& running,
        std::array<std::size_t, Graph::size>& ready) noexcept {
        std::size_t count = 0;
        for (std::size_t index = 0; index < Graph::size; ++index) {
            const auto& state = graph.state(index);
            if (!running[index] && !state.completed &&
                !state.blocked_by_commands && state.pending_predecessors == 0 &&
                !Graph::is_individual[index]) {
                ready[count++] = index;
            }
        }
        if (count != 0) {
            return count;
        }
        for (std::size_t index = 0; index < Graph::size; ++index) {
            const auto& state = graph.state(index);
            if (!running[index] && !state.completed &&
                !state.blocked_by_commands && state.pending_predecessors == 0) {
                ready[0] = index;
                return 1;
            }
        }
        return 0;
    }

    template <typename Graph>
    void _assign_command_buffers(
        const std::array<std::size_t, Graph::size>& ready,
        std::array<command_buffer*, Graph::size>& buffers_by_task,
        std::size_t ready_count, std::size_t& dirty_buffer_count) noexcept {
        for (std::size_t index = 0; index < ready_count; ++index) {
            const auto task_index = ready[index];
            if constexpr (Graph::buffered_command_node_count != 0) {
                if (Graph::has_buffered_commands[task_index]) {
                    buffers_by_task[task_index] =
                        std::addressof(buffers_[dirty_buffer_count++]);
                    continue;
                }
            }
            buffers_by_task[task_index] = nullptr;
        }
    }

    template <stage Stage, typename Graph, execution::scheduler Sch>
    void _run_stage(Sch& sch) {
        static_assert(Graph::value, "Invalid ECS stage graph");
        if constexpr (Graph::size == 0) {
            return;
        } else {
            stage_task_graph<Graph> graph;
            std::array<bool, Graph::size> running{};
            std::array<std::size_t, Graph::size> ready{};
            std::array<command_buffer*, Graph::size> buffers_by_task{};
            _completion_queue<Graph::size> completions;
            std::size_t dirty_buffer_count = 0;
            std::size_t inflight           = 0;

            using sender_type =
                decltype(execution::schedule(std::declval<Sch&>()) | execution::then(_task_fn<Stage, world_type>{}));
            using operation_type = _connected_task<sender_type, Graph::size>;
            std::array<std::optional<operation_type>, Graph::size> operations{};

            while (!graph.done()) {
                const auto ready_count =
                    _collect_runnable<Graph>(graph, running, ready);
                if (ready_count != 0) {
                    _assign_command_buffers<Graph>(
                        ready, buffers_by_task, ready_count,
                        dirty_buffer_count);
                    for (std::size_t index = 0; index < ready_count; ++index) {
                        const auto task_index = ready[index];
                        auto sender =
                            execution::schedule(sch) |
                            execution::then(
                                _task_fn<Stage, world_type>{
                                    std::addressof(world_), task_index,
                                    buffers_by_task[task_index] });
                        operations[task_index].emplace(
                            std::move(sender), completions, task_index);
                        operations[task_index]->start();
                        running[task_index] = true;
                        ++inflight;
                    }
                    continue;
                }

                if (graph.needs_flush() && inflight == 0) {
                    flush(dirty_buffer_count);
                    dirty_buffer_count = 0;
                    graph.flush();
                    continue;
                }

                if (inflight == 0) {
                    break;
                }

                auto completed = completions.wait_pop();
                _complete_task(graph, running, operations, completed, inflight);

                while (completions.try_pop(completed)) {
                    _complete_task(
                        graph, running, operations, completed, inflight);
                }
            }

            if (graph.dirty_commands()) {
                flush(dirty_buffer_count);
                graph.flush();
            }
        }
    }

    template <typename Graph, typename Operations>
    static void _complete_task(
        stage_task_graph<Graph>& graph, std::array<bool, Graph::size>& running,
        Operations& operations,
        const typename _completion_queue<Graph::size>::record& completed,
        std::size_t& inflight) {
        running[completed.index] = false;
        operations[completed.index].reset();
        --inflight;
        if (completed.error) {
            std::rethrow_exception(completed.error);
        }
        graph.complete(completed.index);
    }

    template <stage Stage, typename Graph>
    void _step_stage() {
        static_assert(Graph::value, "Invalid ECS stage graph");
        if constexpr (Graph::size == 0) {
            return;
        } else {
            stage_task_graph<Graph> graph;
            std::array<bool, Graph::size> running{};
            std::array<std::size_t, Graph::size> ready{};
            std::array<command_buffer*, Graph::size> buffers_by_task{};
            std::size_t dirty_buffer_count = 0;

            while (!graph.done()) {
                const auto ready_count =
                    _collect_runnable<Graph>(graph, running, ready);
                if (ready_count == 0) {
                    if (!graph.needs_flush()) {
                        break;
                    }
                    flush(dirty_buffer_count);
                    dirty_buffer_count = 0;
                    graph.flush();
                    continue;
                }

                _assign_command_buffers<Graph>(
                    ready, buffers_by_task, ready_count, dirty_buffer_count);
                for (std::size_t index = 0; index < ready_count; ++index) {
                    const auto task_index = ready[index];
                    world_accessor::call_task<Stage>(
                        world_, task_index, buffers_by_task[task_index]);
                    graph.complete(task_index);
                }
            }

            if (graph.dirty_commands()) {
                flush(dirty_buffer_count);
                graph.flush();
            }
        }
    }

    ATOM_NO_UNIQUE_ADDR byte_alloc alloc_;
    world_type world_;
    buffer_vector buffers_;
};

} // namespace _runtime

template <
    execution::scheduler Sch, typename RunEnvs, typename Payload,
    typename Alloc = std::allocator<std::byte>>
class runtime {
    using run_envs    = RunEnvs;
    using env_tuple   = type_list_rebind_t<shared_tuple, run_envs>;
    using pack_traits = _runtime::_run_env_pack_traits<run_envs>;

public:
    static constexpr std::size_t run_env_count = std::tuple_size_v<env_tuple>;

    static_assert(
        pack_traits::events_enabled_count <= 1,
        "runtime allows at most one world marked with enable_events");
    static_assert(
        pack_traits::render_enabled_count <= 1,
        "runtime allows at most one world marked with enable_render");

    runtime(Sch& sch, Payload* payload) : runtime(sch, payload, Alloc{}) {}

    runtime(Sch&& sch, Payload* payload)
        : runtime(std::move(sch), payload, Alloc{}) {}

    template <typename Al = Alloc>
    runtime(Sch& sch, Payload* payload, const Al& alloc)
        : sch_(sch), payload_(payload), alloc_(alloc),
          envs_(_make_envs(alloc_, std::make_index_sequence<run_env_count>{})) {
    }

    template <typename Al = Alloc>
    runtime(Sch&& sch, Payload* payload, const Al& alloc)
        : sch_(std::move(sch)), payload_(payload), alloc_(alloc),
          envs_(_make_envs(alloc_, std::make_index_sequence<run_env_count>{})) {
    }

    template <std::size_t Index>
    [[nodiscard]] auto run_env() noexcept -> decltype(auto) {
        return envs_.template get<Index>();
    }

    template <std::size_t Index>
    [[nodiscard]] auto run_env() const noexcept -> decltype(auto) {
        return envs_.template get<Index>();
    }

    int run() {
        using namespace execution;
        if (payload_ == nullptr) [[unlikely]] {
            throw std::runtime_error("runtime payload is nullptr");
        }

        const auto guarantee = get_forward_progress_guarantee(sch_);
        if (guarantee == forward_progress_guarantee::weakly_parallel) {
            _run_weak_parallel();
        } else {
            _run_blocking();
        }

        return 0;
    }

private:
    template <std::size_t... Is>
    static auto _make_envs(const Alloc& alloc, std::index_sequence<Is...>)
        -> env_tuple {
        return env_tuple{ std::tuple_element_t<Is, env_tuple>{ alloc }... };
    }

    template <typename Fn>
    void _for_each_env(Fn&& fn) {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (fn(envs_.template get<Is>()), ...);
        }(std::make_index_sequence<run_env_count>{});
    }

    [[nodiscard]] bool _poll_events() {
        if constexpr (_runtime::_poll_events_with_runtime<Payload, runtime>) {
            return payload_->poll_events(*this);
        } else if constexpr (_runtime::_poll_events_plain<Payload>) {
            return payload_->poll_events();
        } else {
            return false;
        }
    }

    [[nodiscard]] bool _is_stopped() {
        if constexpr (_runtime::_is_stopped_with_runtime<Payload, runtime>) {
            return payload_->is_stopped(*this);
        } else if constexpr (_runtime::_is_stopped_plain<Payload>) {
            return payload_->is_stopped();
        } else {
            return false;
        }
    }

    void _render_begin() {
        if constexpr (_runtime::_render_begin_with_runtime<Payload, runtime>) {
            payload_->render_begin(*this);
        } else if constexpr (_runtime::_render_begin_plain<Payload>) {
            payload_->render_begin();
        }
    }

    void _render_end() {
        if constexpr (_runtime::_render_end_with_runtime<Payload, runtime>) {
            payload_->render_end(*this);
        } else if constexpr (_runtime::_render_end_plain<Payload>) {
            payload_->render_end();
        }
    }

    void _run_weak_parallel() {
        static volatile std::sig_atomic_t keep_running = 1;
        keep_running                                   = 1;
        auto exit = [](int) noexcept { keep_running = 0; };
        std::signal(SIGINT, exit);
        std::signal(SIGTERM, exit);

        _for_each_env([this](auto& env) { env.startup_step(); });
        while (keep_running != 0 && !_is_stopped()) {
            if (_poll_events()) {
                continue;
            }
            if constexpr (pack_traits::events_enabled_count != 0) {
                _for_each_env([](auto& env) { env.events_step(); });
            }
            _for_each_env([](auto& env) { env.update_step(); });
            if constexpr (pack_traits::render_enabled_count != 0) {
                _render_begin();
                _for_each_env([](auto& env) { env.render_step(); });
                _render_end();
            }
        }
        _for_each_env([](auto& env) { env.shutdown_step(); });
    }

    void _run_blocking() {
        _for_each_env([this](auto& env) { env.startup(sch_); });
        while (!_is_stopped()) {
            if (_poll_events()) {
                continue;
            }
            if constexpr (pack_traits::events_enabled_count != 0) {
                _for_each_env([this](auto& env) { env.events(sch_); });
            }
            _for_each_env([this](auto& env) { env.update(sch_); });
            if constexpr (pack_traits::render_enabled_count != 0) {
                _render_begin();
                _for_each_env([this](auto& env) { env.render(sch_); });
                _render_end();
            }
        }
        _for_each_env([this](auto& env) { env.shutdown(sch_); });
    }

    Sch sch_;
    Payload* payload_;
    ATOM_NO_UNIQUE_ADDR Alloc alloc_;
    env_tuple envs_;
};

template <auto... Worlds, execution::scheduler Sch>
ATOM_NODISCARD auto make_runtime([[maybe_unused]] Sch&&, std::nullptr_t) {
    static_assert(
        sizeof...(Worlds) == static_cast<std::size_t>(-1),
        "application payload is required");
}

template <
    auto... Worlds, execution::scheduler Sch, typename Payload, typename Alloc>
ATOM_NODISCARD auto
    make_runtime(Sch&& sch, Payload* payload, const Alloc& alloc) {
    using byte_alloc = rebind_alloc_t<Alloc, std::byte>;
    using run_envs   = _make_run_envs_t<byte_alloc, _to_run_envs_t<Worlds...>>;
    return runtime<std::decay_t<Sch>, run_envs, Payload, byte_alloc>(
        std::forward<Sch>(sch), payload, byte_alloc{ alloc });
}

template <auto... Worlds, execution::scheduler Sch, typename Payload>
ATOM_NODISCARD auto make_runtime(Sch&& sch, Payload* payload) {
    return make_runtime<Worlds...>(
        std::forward<Sch>(sch), payload, std::allocator<std::byte>{});
}

template <std::size_t GroupID, typename Alloc, auto... Worlds>
class run_env_for_group {
    template <auto World>
    using world_state =
        _runtime::_world_state<std::remove_cvref_t<decltype(World)>, Alloc>;

    using worlds_tuple = shared_tuple<world_state<Worlds>...>;

public:
    static constexpr std::size_t world_count = sizeof...(Worlds);

    static constexpr std::size_t max_concurrency =
        _runtime::_common_max(get_max_concurrency(Worlds)...);

    static constexpr std::size_t total_max_concurrency =
        (get_max_concurrency(Worlds) + ...);

    static constexpr std::size_t events_enabled_count =
        (std::size_t{ 0 } + ... + (get_events(Worlds) ? 1U : 0U));

    static constexpr std::size_t render_enabled_count =
        (std::size_t{ 0 } + ... + (get_render(Worlds) ? 1U : 0U));

    explicit run_env_for_group(const Alloc& alloc = {})
        : worlds_(
              _make_worlds(alloc, std::make_index_sequence<world_count>{})) {}

    run_env_for_group(const run_env_for_group&)            = delete;
    run_env_for_group(run_env_for_group&&)                 = default;
    run_env_for_group& operator=(const run_env_for_group&) = delete;
    run_env_for_group& operator=(run_env_for_group&&)      = delete;
    ~run_env_for_group()                                   = default;

    template <std::size_t Index>
    [[nodiscard]] auto world() noexcept -> decltype(auto) {
        return worlds_.template get<Index>().world();
    }

    template <std::size_t Index>
    [[nodiscard]] auto world() const noexcept -> decltype(auto) {
        return worlds_.template get<Index>().world();
    }

    template <execution::scheduler Sch>
    void startup(Sch& sch) {
        _run_all<stage::pre_startup>(sch);
        _run_all<stage::startup>(sch);
        _run_all<stage::post_startup>(sch);
        _run_all<stage::first>(sch);
    }

    void startup_step() {
        _step_all<stage::pre_startup>();
        _step_all<stage::startup>();
        _step_all<stage::post_startup>();
        _step_all<stage::first>();
    }

    template <execution::scheduler Sch>
    void events(Sch& sch) {
        _run_all<stage::events>(sch);
    }

    void events_step() { _step_all<stage::events>(); }

    template <execution::scheduler Sch>
    void update(Sch& sch) {
        _update_all(sch);
    }

    void update_step() { _update_step_all(); }

    template <execution::scheduler Sch>
    void render(Sch& sch) {
        _run_all<stage::render>(sch);
    }

    void render_step() { _step_all<stage::render>(); }

    template <execution::scheduler Sch>
    void shutdown(Sch& sch) {
        _run_all<stage::last>(sch);
        _run_all<stage::shutdown>(sch);
    }

    void shutdown_step() {
        _step_all<stage::last>();
        _step_all<stage::shutdown>();
    }

private:
    template <std::size_t... Is>
    static auto _make_worlds(const Alloc& alloc, std::index_sequence<Is...>)
        -> worlds_tuple {
        return worlds_tuple{ std::tuple_element_t<Is, worlds_tuple>{
            alloc }... };
    }

    template <typename Fn>
    void _for_each_world(Fn&& fn) {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (fn(worlds_.template get<Is>()), ...);
        }(std::make_index_sequence<world_count>{});
    }

    template <stage Stage, execution::scheduler Sch>
    void _run_all(Sch& sch) {
        _for_each_world(
            [&sch](auto& state) { state.template run_stage<Stage>(sch); });
    }

    template <stage Stage>
    void _step_all() {
        _for_each_world(
            [](auto& state) { state.template step_stage<Stage>(); });
    }

    template <execution::scheduler Sch>
    void _update_all(Sch& sch) {
        _for_each_world([&sch](auto& state) {
            if (state.world().should_call_update()) {
                state.template run_stage<stage::pre_update>(sch);
                state.template run_stage<stage::update>(sch);
                state.template run_stage<stage::post_update>(sch);
            }
        });
    }

    void _update_step_all() {
        _for_each_world([](auto& state) {
            if (state.world().should_call_update()) {
                state.template step_stage<stage::pre_update>();
                state.template step_stage<stage::update>();
                state.template step_stage<stage::post_update>();
            }
        });
    }

    worlds_tuple worlds_;
};

template <typename Alloc, auto World>
class alignas(std::hardware_destructive_interference_size)
    run_env_for_individual {
    using state_type =
        _runtime::_world_state<std::remove_cvref_t<decltype(World)>, Alloc>;

public:
    static constexpr std::size_t max_concurrency       = 1;
    static constexpr std::size_t total_max_concurrency = 1;
    static constexpr std::size_t events_enabled_count =
        get_events(World) ? 1U : 0U;
    static constexpr std::size_t render_enabled_count =
        get_render(World) ? 1U : 0U;

    explicit run_env_for_individual(const Alloc& alloc = {}) : state_(alloc) {}

    run_env_for_individual(const run_env_for_individual&)            = delete;
    run_env_for_individual(run_env_for_individual&&)                 = default;
    run_env_for_individual& operator=(const run_env_for_individual&) = delete;
    run_env_for_individual& operator=(run_env_for_individual&&)      = delete;
    ~run_env_for_individual()                                        = default;

    [[nodiscard]] auto world() noexcept -> typename state_type::world_type& {
        return state_.world();
    }

    [[nodiscard]] auto world() const noexcept -> const
        typename state_type::world_type& {
        return state_.world();
    }

    template <execution::scheduler Sch>
    void startup(Sch& sch) {
        state_.template run_stage<stage::pre_startup>(sch);
        state_.template run_stage<stage::startup>(sch);
        state_.template run_stage<stage::post_startup>(sch);
        state_.template run_stage<stage::first>(sch);
    }

    void startup_step() {
        state_.template step_stage<stage::pre_startup>();
        state_.template step_stage<stage::startup>();
        state_.template step_stage<stage::post_startup>();
        state_.template step_stage<stage::first>();
    }

    template <execution::scheduler Sch>
    void events(Sch& sch) {
        state_.template run_stage<stage::events>(sch);
    }

    void events_step() { state_.template step_stage<stage::events>(); }

    template <execution::scheduler Sch>
    void update(Sch& sch) {
        if (state_.world().should_call_update()) {
            state_.template run_stage<stage::pre_update>(sch);
            state_.template run_stage<stage::update>(sch);
            state_.template run_stage<stage::post_update>(sch);
        }
    }

    void update_step() {
        if (state_.world().should_call_update()) {
            state_.template step_stage<stage::pre_update>();
            state_.template step_stage<stage::update>();
            state_.template step_stage<stage::post_update>();
        }
    }

    template <execution::scheduler Sch>
    void render(Sch& sch) {
        state_.template run_stage<stage::render>(sch);
    }

    void render_step() { state_.template step_stage<stage::render>(); }

    template <execution::scheduler Sch>
    void shutdown(Sch& sch) {
        state_.template run_stage<stage::last>(sch);
        state_.template run_stage<stage::shutdown>(sch);
    }

    void shutdown_step() {
        state_.template step_stage<stage::last>();
        state_.template step_stage<stage::shutdown>();
    }

private:
    state_type state_;
};

} // namespace neutron
