#pragma once
#include <array>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <csignal>
#include <cstddef>
#include <cstdint>
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
#include "neutron/detail/ecs/task_graph.hpp"
#include "neutron/detail/ecs/world.hpp"
#include "neutron/detail/ecs/world_accessor.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_events.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_execution_policy.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_max_buffer_count.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_max_concurrency.hpp"
#include "neutron/detail/ecs/world_descriptor/queries/get_render.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/memory/rebind_alloc.hpp"
#include "neutron/detail/tuple/shared_tuple.hpp"
#include "neutron/execution.hpp"
#include "neutron/metafn.hpp"

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
        std::size_t task_index{};
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
    std::size_t task_index;

    void set_value() && noexcept { queue->push({ .task_index = task_index }); }

    template <typename Error>
    void set_error(Error&& error) && noexcept {
        if constexpr (std::same_as<
                          std::remove_cvref_t<Error>, std::exception_ptr>) {
            queue->push(
                { .task_index = task_index,
                  .error      = std::forward<Error>(error) });
        } else {
            queue->push(
                { .task_index = task_index,
                  .error =
                      std::make_exception_ptr(std::forward<Error>(error)) });
        }
    }

    void set_stopped() && noexcept {
        queue->push({ .task_index = task_index, .stopped = true });
    }

    ATOM_NODISCARD constexpr auto get_env() const noexcept {
        return execution::env<>{};
    }
};

template <typename Sender, std::size_t Size>
struct _connected_task {
    using opstate_type = decltype(execution::connect(
        std::declval<Sender>(), std::declval<_completion_receiver<Size>>()));

    template <typename Sndr>
    _connected_task(
        Sndr&& sender, _completion_queue<Size>& queue, std::size_t task_index)
        : opstate(
              execution::connect(
                  std::forward<Sndr>(sender),
                  _completion_receiver<Size>{ &queue, task_index })) {}

    void start() noexcept { execution::start(opstate); }

    opstate_type opstate;
};

template <typename Descriptor, typename Alloc>
class _world_slot {
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
    template <stage Stage>
    using task_set = decltype(world_type::template get_tasks<Stage>());

    template <stage Stage>
    using graph = _stage_task_graph<task_set<Stage>>;

    explicit _world_slot(const Alloc& alloc = {})
        : world_(byte_alloc{ alloc }),
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

    [[nodiscard]] bool should_call_update() noexcept {
        return world_.should_call_update();
    }

    [[nodiscard]] auto buffer(std::size_t index) noexcept -> command_buffer& {
        assert(index < buffers_.size());
        return buffers_[index];
    }

    void flush(std::size_t count) {
        assert(count <= buffers_.size());
        for (std::size_t index = 0; index < count; ++index) {
            buffers_[index].apply(world_accessor::base(world_));
            buffers_[index].reset();
        }
    }

    static constexpr std::uint32_t max_buffer_count =
        get_max_buffer_count(Descriptor{});

private:
    world_type world_;
    buffer_vector buffers_;
};

template <stage Stage, typename SlotsTuple, typename IndexSeq>
struct _env_graph_for_slots;

template <stage Stage, typename SlotsTuple, std::size_t... Is>
struct _env_graph_for_slots<Stage, SlotsTuple, std::index_sequence<Is...>> {
    using type = _env_task_graph<
        decltype(std::tuple_element_t<
                 Is, SlotsTuple>::world_type::template get_tasks<Stage>())...>;
};

template <stage Stage, typename SlotsTuple>
using _env_graph_for_slots_t = typename _env_graph_for_slots<
    Stage, SlotsTuple,
    std::make_index_sequence<
        std::tuple_size_v<std::remove_cvref_t<SlotsTuple>>>>::type;

template <std::size_t Index = 0, typename Tuple, typename Fn>
void _visit_tuple(Tuple& tuple, std::size_t index, Fn&& fn) {
    if constexpr (Index < std::tuple_size_v<std::remove_cvref_t<Tuple>>) {
        if (index == Index) {
            std::forward<Fn>(fn)(
                std::integral_constant<std::size_t, Index>{},
                tuple.template get<Index>());
        } else {
            _visit_tuple<Index + 1>(tuple, index, std::forward<Fn>(fn));
        }
    } else {
        assert(false);
    }
}

template <typename TaskSet, std::size_t Index = 0, typename Fn>
void _visit_task(std::size_t index, Fn&& fn) {
    if constexpr (Index < _stage_task_graph<TaskSet>::task_count) {
        if (index == Index) {
            std::forward<Fn>(fn)(std::integral_constant<std::size_t, Index>{});
        } else {
            _visit_task<TaskSet, Index + 1>(index, std::forward<Fn>(fn));
        }
    } else {
        assert(false);
    }
}

template <typename EnvGraph, typename SlotsTuple>
struct _env_task_fn {
    using slots_type = std::remove_cvref_t<SlotsTuple>;
    using command_buffer =
        typename std::tuple_element_t<0, slots_type>::command_buffer;

    slots_type* slots;
    std::size_t task_index;
    command_buffer* cmdbuf;

    void operator()() const {
        const auto world_index = EnvGraph::world_indices[task_index];
        const auto local_index = EnvGraph::local_task_indices[task_index];

        _visit_tuple(*slots, world_index, [&](auto world_constant, auto& slot) {
            constexpr auto world = decltype(world_constant)::value;
            using task_set       = typename EnvGraph::template task_set<world>;

            _visit_task<task_set>(local_index, [&](auto task_constant) {
                constexpr auto task = decltype(task_constant)::value;
                _task_graph::_task_invoker<task_set, task>::call(
                    slot.world(), cmdbuf);
            });
        });
    }
};

template <typename EnvGraph, typename SlotsTuple>
auto _assign_buffer(
    SlotsTuple& slots,
    std::array<std::size_t, EnvGraph::world_count>& dirty_buffer_counts,
    std::size_t task_index) ->
    typename std::tuple_element_t<
        0, std::remove_cvref_t<SlotsTuple>>::command_buffer* {
    using command_buffer = typename std::tuple_element_t<
        0, std::remove_cvref_t<SlotsTuple>>::command_buffer;
    command_buffer* result = nullptr;

    if constexpr (EnvGraph::buffered_command_node_count != 0) {
        if (EnvGraph::has_buffered_commands[task_index]) {
            const auto world = EnvGraph::world_indices[task_index];
            _visit_tuple(slots, world, [&](auto, auto& slot) {
                result =
                    std::addressof(slot.buffer(dirty_buffer_counts[world]++));
            });
        }
    }

    return result;
}

template <typename EnvGraph, typename SlotsTuple>
void _flush_world(
    SlotsTuple& slots,
    std::array<std::size_t, EnvGraph::world_count>& dirty_buffer_counts,
    _env_task_executor<EnvGraph>& executor, std::size_t world) {
    _visit_tuple(slots, world, [&](auto, auto& slot) {
        slot.flush(dirty_buffer_counts[world]);
    });
    dirty_buffer_counts[world] = 0;
    executor.flush(world);
}

template <stage Stage, execution::scheduler Sch, typename SlotsTuple>
void _run_env_stage(
    Sch& sch, SlotsTuple& slots,
    const std::array<bool, std::tuple_size_v<std::remove_cvref_t<SlotsTuple>>>*
        active = nullptr) {
    using slots_type = std::remove_cvref_t<SlotsTuple>;
    using graph      = _env_graph_for_slots_t<Stage, slots_type>;

    if constexpr (graph::task_count != 0) {
        static constexpr std::size_t queue_size = graph::task_count;
        using task_fn = _env_task_fn<graph, slots_type>;
        using sender_type =
            decltype(execution::schedule(std::declval<Sch&>()) | execution::then(std::declval<task_fn>()));
        using operation_type = _connected_task<sender_type, queue_size>;

        _env_task_executor<graph> executor;
        executor.reset(active);

        std::array<std::size_t, queue_size> ready{};
        std::array<std::optional<operation_type>, queue_size> operations{};
        std::array<std::size_t, graph::world_count> dirty_buffer_counts{};
        _completion_queue<queue_size> completions;
        std::size_t inflight = 0;

        auto process_completion = [&](auto completed) {
            const auto task = completed.task_index;
            --inflight;
            executor.complete(task);
            operations[task].reset();
            if (completed.error) {
                std::rethrow_exception(completed.error);
            }
        };

        while (!executor.done()) {
            const auto ready_count = executor.collect_runnable(ready);
            if (ready_count != 0) {
                for (std::size_t index = 0; index < ready_count; ++index) {
                    const auto task = ready[index];
                    auto* cmdbuf =
                        _assign_buffer<graph>(slots, dirty_buffer_counts, task);
                    auto sender =
                        execution::schedule(sch) |
                        execution::then(task_fn{ &slots, task, cmdbuf });
                    operations[task].emplace(
                        std::move(sender), completions, task);
                    executor.mark_running(task);
                    ++inflight;
                    operations[task]->start();
                }
                continue;
            }

            bool flushed = false;
            for (std::size_t world = 0; world < graph::world_count; ++world) {
                if (executor.needs_flush(world)) {
                    _flush_world<graph>(
                        slots, dirty_buffer_counts, executor, world);
                    flushed = true;
                }
            }
            if (flushed) {
                continue;
            }

            if (inflight == 0) {
                break;
            }

            auto completed = completions.wait_pop();
            process_completion(completed);
            while (completions.try_pop(completed)) {
                process_completion(completed);
            }
        }

        for (std::size_t world = 0; world < graph::world_count; ++world) {
            if (executor.dirty(world)) {
                _flush_world<graph>(
                    slots, dirty_buffer_counts, executor, world);
            }
        }
    }
}

template <std::size_t Count>
struct _update_mask {
    std::array<bool, Count> active{};
    bool any = false;
};

template <typename SlotsTuple, std::size_t... Is>
auto _make_update_mask_impl(SlotsTuple& slots, std::index_sequence<Is...>)
    -> _update_mask<sizeof...(Is)> {
    _update_mask<sizeof...(Is)> mask{};
    ((mask.active[Is] = slots.template get<Is>().should_call_update(),
      mask.any        = mask.any || mask.active[Is]),
     ...);
    return mask;
}

template <typename SlotsTuple>
auto _make_update_mask(SlotsTuple& slots)
    -> _update_mask<std::tuple_size_v<std::remove_cvref_t<SlotsTuple>>> {
    return _make_update_mask_impl(
        slots, std::make_index_sequence<
                   std::tuple_size_v<std::remove_cvref_t<SlotsTuple>>>{});
}

template <typename Payload, typename Runtime>
constexpr bool _poll_events(Payload& payload, Runtime& runtime) {
    if constexpr (_poll_events_with_runtime<Payload, Runtime>) {
        return payload.poll_events(runtime);
    } else if constexpr (_poll_events_plain<Payload>) {
        return payload.poll_events();
    } else {
        return true;
    }
}

template <typename Payload, typename Runtime>
constexpr bool _is_stopped(Payload& payload, Runtime& runtime) {
    if constexpr (_is_stopped_with_runtime<Payload, Runtime>) {
        return payload.is_stopped(runtime);
    } else if constexpr (_is_stopped_plain<Payload>) {
        return payload.is_stopped();
    } else {
        return false;
    }
}

template <typename Payload, typename Runtime>
constexpr void _render_begin(Payload& payload, Runtime& runtime) {
    if constexpr (_render_begin_with_runtime<Payload, Runtime>) {
        payload.render_begin(runtime);
    } else if constexpr (_render_begin_plain<Payload>) {
        payload.render_begin();
    }
}

template <typename Payload, typename Runtime>
constexpr void _render_end(Payload& payload, Runtime& runtime) {
    if constexpr (_render_end_with_runtime<Payload, Runtime>) {
        payload.render_end(runtime);
    } else if constexpr (_render_end_plain<Payload>) {
        payload.render_end();
    }
}

} // namespace _runtime

template <
    execution::scheduler Sch, typename RunEnvs, typename Payload,
    typename Alloc = std::allocator<std::byte>>
class runtime {
    using run_envs    = RunEnvs;
    using env_tuple   = type_list_rebind_t<shared_tuple, run_envs>;
    using pack_traits = _runtime::_run_env_pack_traits<run_envs>;

    inline static std::atomic<bool> running = true;

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
        running.store(true, std::memory_order_release);

        constexpr auto exit = [](int) noexcept {
            running.store(false, std::memory_order_release);
        };
        std::signal(SIGINT, exit);
        std::signal(SIGTERM, exit);

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

    template <stage Stage, std::size_t... Is>
    void _run_stage_all(std::index_sequence<Is...>) {
        using namespace this_thread;
        using namespace execution;
        sync_wait(when_all(
            envs_.template get<Is>().template stage_parallel<Stage>(sch_)...));
    }

    template <stage Stage>
    void _run_stage_all() {
        _run_stage_all<Stage>(
            std::make_index_sequence<std::tuple_size_v<env_tuple>>{});
    }

    template <std::size_t... Is>
    void _run_update_all(std::index_sequence<Is...>) {
        using namespace this_thread;
        using namespace execution;
        sync_wait(when_all(envs_.template get<Is>().update_parallel(sch_)...));
    }

    void _run_update_all() {
        _run_update_all(
            std::make_index_sequence<std::tuple_size_v<env_tuple>>{});
    }

    void _run_startup_all() {
        _run_stage_all<stage::pre_startup>();
        _run_stage_all<stage::startup>();
        _run_stage_all<stage::post_startup>();
        _run_stage_all<stage::first>();
    }

    void _run_shutdown_all() {
        _run_stage_all<stage::last>();
        _run_stage_all<stage::shutdown>();
    }

    void _run_frame() {
        if (!_runtime::_poll_events(*payload_, *this)) {
            if (_runtime::_is_stopped(*payload_, *this)) {
                running.store(false, std::memory_order_release);
            }
            return;
        }

        _run_stage_all<stage::events>();
        if (_runtime::_is_stopped(*payload_, *this)) {
            running.store(false, std::memory_order_release);
            return;
        }

        _run_update_all();
        if constexpr (pack_traits::render_enabled_count != 0) {
            _runtime::_render_begin(*payload_, *this);
            _run_stage_all<stage::render>();
            _runtime::_render_end(*payload_, *this);
        }
    }

    void _run_weak_parallel() { _run_blocking(); }

    void _run_blocking() {
        _run_startup_all();
        while (running.load(std::memory_order_acquire)) {
            _run_frame();
        }
        _run_shutdown_all();
    }

    Sch sch_;
    Payload* payload_;
    ATOM_NO_UNIQUE_ADDR Alloc alloc_;
    env_tuple envs_;
};

template <auto... Worlds, execution::scheduler Sch, typename Payload>
requires(sizeof...(Worlds) == 0)
ATOM_NODISCARD auto make_runtime([[maybe_unused]] Sch&&, Payload*) {
    static_assert(false, "world count could not be zero");
}

template <auto... Worlds, execution::scheduler Sch>
ATOM_NODISCARD auto make_runtime([[maybe_unused]] Sch&&, std::nullptr_t) {
    static_assert(false, "application payload is invalid");
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
    using world_slot =
        _runtime::_world_slot<std::remove_cvref_t<decltype(World)>, Alloc>;

    using worlds_tuple = shared_tuple<world_slot<Worlds>...>;

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
        : worlds_(_make_worlds(
              alloc, std::make_index_sequence<sizeof...(Worlds)>{})) {}

    run_env_for_group(const run_env_for_group&)            = delete;
    run_env_for_group(run_env_for_group&&)                 = default;
    run_env_for_group& operator=(const run_env_for_group&) = delete;
    run_env_for_group& operator=(run_env_for_group&&)      = delete;
    ~run_env_for_group()                                   = default;

    template <std::size_t Index>
    requires(Index <= sizeof...(Worlds))
    [[nodiscard]] auto get_world() noexcept -> decltype(auto) {
        return worlds_.template get<Index>().world();
    }

    template <std::size_t Index>
    requires(Index <= sizeof...(Worlds))
    [[nodiscard]] auto get_world() const noexcept -> decltype(auto) {
        return worlds_.template get<Index>().world();
    }

    template <stage Stage, typename Sch>
    auto stage_parallel(Sch& sch) {
        using namespace execution;
        auto sndr = [this, &sch] {
            _runtime::_run_env_stage<Stage>(sch, worlds_);
        };
        return schedule(sch) | then(std::move(sndr));
    }

    template <typename Sch>
    auto update_parallel(Sch& sch) {
        using namespace execution;
        auto sndr = [this, &sch] {
            auto mask = _runtime::_make_update_mask(worlds_);
            if (!mask.any) {
                return;
            }
            _runtime::_run_env_stage<stage::pre_update>(
                sch, worlds_, std::addressof(mask.active));
            _runtime::_run_env_stage<stage::update>(
                sch, worlds_, std::addressof(mask.active));
            _runtime::_run_env_stage<stage::post_update>(
                sch, worlds_, std::addressof(mask.active));
        };
        return schedule(sch) | then(std::move(sndr));
    }

    template <typename Sch>
    auto startup_parallel(Sch& sch) {
        using namespace execution;
        auto sndr = [this, &sch] {
            _runtime::_run_env_stage<stage::pre_startup>(sch, worlds_);
            _runtime::_run_env_stage<stage::startup>(sch, worlds_);
            _runtime::_run_env_stage<stage::post_startup>(sch, worlds_);
            _runtime::_run_env_stage<stage::first>(sch, worlds_);
        };
        return schedule(sch) | then(std::move(sndr));
    }

    template <typename Sch>
    auto shutdown_parallel(Sch& sch) {
        using namespace execution;
        auto sndr = [this, &sch] {
            _runtime::_run_env_stage<stage::last>(sch, worlds_);
            _runtime::_run_env_stage<stage::shutdown>(sch, worlds_);
        };
        return schedule(sch) | then(std::move(sndr));
    }

private:
    template <std::size_t... Is>
    static auto _make_worlds(const Alloc& alloc, std::index_sequence<Is...>)
        -> worlds_tuple {
        return worlds_tuple{ std::tuple_element_t<Is, worlds_tuple>{
            alloc }... };
    }

    worlds_tuple worlds_;
};

template <typename Alloc, auto World>
class alignas(std::hardware_destructive_interference_size)
    run_env_for_individual {
    using slot_type =
        _runtime::_world_slot<std::remove_cvref_t<decltype(World)>, Alloc>;
    using worlds_tuple = shared_tuple<slot_type>;

public:
    static constexpr std::size_t world_count     = 1;
    static constexpr std::size_t max_concurrency = get_max_concurrency(World);
    static constexpr std::size_t total_max_concurrency =
        get_max_concurrency(World);
    static constexpr std::size_t events_enabled_count =
        get_events(World) ? 1U : 0U;
    static constexpr std::size_t render_enabled_count =
        get_render(World) ? 1U : 0U;

    explicit run_env_for_individual(const Alloc& alloc = {})
        : worlds_(slot_type{ alloc }) {}

    run_env_for_individual(const run_env_for_individual&)            = delete;
    run_env_for_individual(run_env_for_individual&&)                 = default;
    run_env_for_individual& operator=(const run_env_for_individual&) = delete;
    run_env_for_individual& operator=(run_env_for_individual&&)      = delete;
    ~run_env_for_individual()                                        = default;

    template <std::size_t Index>
    requires(Index == 0)
    [[nodiscard]] auto get_world() noexcept -> typename slot_type::world_type& {
        return worlds_.template get<0>().world();
    }

    template <std::size_t Index>
    requires(Index == 0)
    [[nodiscard]] auto get_world() const noexcept -> const
        typename slot_type::world_type& {
        return worlds_.template get<0>().world();
    }

    template <stage Stage, typename Sch>
    auto stage_parallel(Sch& sch) {
        using namespace execution;
        auto sndr = [this, &sch] {
            _runtime::_run_env_stage<Stage>(sch, worlds_);
        };
        return schedule(sch) | then(std::move(sndr));
    }

    template <typename Sch>
    auto update_parallel(Sch& sch) {
        using namespace execution;
        auto sndr = [this, &sch] {
            auto mask = _runtime::_make_update_mask(worlds_);
            if (!mask.any) {
                return;
            }
            _runtime::_run_env_stage<stage::pre_update>(
                sch, worlds_, std::addressof(mask.active));
            _runtime::_run_env_stage<stage::update>(
                sch, worlds_, std::addressof(mask.active));
            _runtime::_run_env_stage<stage::post_update>(
                sch, worlds_, std::addressof(mask.active));
        };
        return schedule(sch) | then(std::move(sndr));
    }

    template <typename Sch>
    auto startup_parallel(Sch& sch) {
        using namespace execution;
        auto sndr = [this, &sch] {
            _runtime::_run_env_stage<stage::pre_startup>(sch, worlds_);
            _runtime::_run_env_stage<stage::startup>(sch, worlds_);
            _runtime::_run_env_stage<stage::post_startup>(sch, worlds_);
            _runtime::_run_env_stage<stage::first>(sch, worlds_);
        };
        return schedule(sch) | then(std::move(sndr));
    }

    template <typename Sch>
    auto shutdown_parallel(Sch& sch) {
        using namespace execution;
        auto sndr = [this, &sch] {
            _runtime::_run_env_stage<stage::last>(sch, worlds_);
            _runtime::_run_env_stage<stage::shutdown>(sch, worlds_);
        };
        return schedule(sch) | then(std::move(sndr));
    }

private:
    worlds_tuple worlds_;
};

} // namespace neutron
