// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <execution>
#include <latch>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>
#include "neutron/detail/ecs/archetype.hpp"
#include "neutron/detail/ecs/command_buffer.hpp"
#include "neutron/detail/ecs/metainfo.hpp"
#include "neutron/detail/ecs/query.hpp"
#include "neutron/detail/ecs/task_graph.hpp"
#include "neutron/detail/ecs/world_base.hpp"
#include "neutron/detail/ecs/world_descriptor.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution_resources/normthread.hpp"
#include "neutron/detail/memory/rebind_alloc.hpp"
#include "neutron/execution.hpp"
#include "neutron/memory.hpp"
#include "neutron/tuple.hpp"

namespace neutron {

namespace _basic_world {

template <typename Descriptor, bool = world_descriptor<Descriptor>>
struct _descriptor_validator;

template <typename Descriptor>
struct _descriptor_validator<Descriptor, true> {
    static_assert(
        descriptor_traits<Descriptor>::value, "Invalid ECS world descriptor");
};

template <typename Descriptor>
struct _descriptor_validator<Descriptor, false> {
    static_assert(
        world_descriptor<Descriptor>,
        "basic_world requires a world descriptor");
};

} // namespace _basic_world

template <std_simple_allocator Alloc>
class basic_world<world_descriptor_t<>, Alloc> : public world_base<Alloc> {
    template <auto, typename, size_t>
    friend struct construct_from_world_t;
    friend struct world_accessor;

    template <typename Ty>
    using _allocator_t = rebind_alloc_t<Alloc, Ty>;

    template <typename Ty>
    using _vector_t = std::vector<Ty, _allocator_t<Ty>>;

public:
    using descriptor_type = world_descriptor_t<>;
    using allocator_type  = Alloc;
    using command_buffer  = ::neutron::command_buffer<Alloc>;
    template <typename... Filters>
    using query_type    = query<Filters...>;
    using commands_type = basic_commands<Alloc>;
    using sysinfo       = sysinfo_holder<world_descriptor_t<>>;
    using queries       = type_list<>;
    // using systems        = world_descriptor_t<>::systems;

    template <typename Al = Alloc>
    constexpr explicit basic_world(const Al& alloc = {})
        : world_base<Alloc>(alloc) {}

    template <stage Stage>
    void call() noexcept {}

    template <stage Stage, neutron::execution::scheduler Sch>
    void call(Sch&, std::span<command_buffer>) noexcept {}

    void set_dynamic_update_interval(double) noexcept {}

    [[nodiscard]] constexpr double dynamic_update_interval() const noexcept {
        return 0.0;
    }

    [[nodiscard]] constexpr bool has_dynamic_update_interval() const noexcept {
        return false;
    }

    [[nodiscard]] constexpr bool should_call_update() noexcept { return true; }

private:
    type_list_rebind_t<neutron::shared_tuple, queries> queries_{};
    std::span<command_buffer> command_buffers_{};
    const insertion_context* insertion_context_ = nullptr;
};

template <typename Descriptor, std_simple_allocator Alloc>
class basic_world :
    private _basic_world::_descriptor_validator<Descriptor>,
    public world_base<Alloc> {
    template <auto, typename, size_t>
    friend struct construct_from_world_t;
    friend struct world_accessor;

    auto _base() & noexcept -> world_base<Alloc>& {
        return *static_cast<world_base<Alloc>*>(this);
    }

    auto _base() const& noexcept -> const world_base<Alloc>& {
        return *static_cast<const world_base<Alloc>*>(this);
    }

    template <typename Ty>
    using _allocator_t = rebind_alloc_t<Alloc, Ty>;
    using _byte_alloc  = _allocator_t<::std::byte>;

    template <typename Ty>
    using _vector_t = ::std::vector<Ty, _allocator_t<Ty>>;

public:
    using descriptor_type = Descriptor;
    using allocator_type  = Alloc;
    using archetype       = ::neutron::archetype<_byte_alloc>;
    using command_buffer  = ::neutron::command_buffer<_byte_alloc>;

    using desc_traits    = descriptor_traits<Descriptor>;
    using execute_traits = execute_info<Descriptor>;
    using sysinfo        = typename desc_traits::sysinfo;
    using grouped        = typename desc_traits::grouped;
    using run_lists      = typename desc_traits::runlists;
    using components     = typename desc_traits::components;
    using locals         = typename desc_traits::locals;
    using queries        = typename desc_traits::queries;
    using resources      = typename desc_traits::resources;
    using clock_type     = std::chrono::steady_clock;

    template <typename Al = Alloc>
    constexpr explicit basic_world(const Al& alloc = {})
        : world_base<Alloc>(alloc) /*, resources_(), locals_()*/ {}

    void set_dynamic_update_interval(double seconds) noexcept {
        if (seconds > 0.0) {
            dynamic_update_interval_     = seconds;
            has_dynamic_update_interval_ = true;
        } else {
            dynamic_update_interval_     = 0.0;
            has_dynamic_update_interval_ = false;
        }
    }

    [[nodiscard]] double dynamic_update_interval() const noexcept {
        return dynamic_update_interval_;
    }

    [[nodiscard]] bool has_dynamic_update_interval() const noexcept {
        return has_dynamic_update_interval_;
    }

    [[nodiscard]] bool should_call_update() noexcept {
        if constexpr (execute_traits::is_always) {
            return true;
        }

        const auto interval = _update_interval();
        if (interval <= 0.0) {
            return false;
        }

        const auto now = clock_type::now();
        if (!has_last_update_ ||
            std::chrono::duration<double>(now - last_update_).count() >=
                interval) {
            last_update_     = now;
            has_last_update_ = true;
            return true;
        }
        return false;
    }

    template <stage Stage>
    void call() {
        using static_graph = stage_graph<Stage, Descriptor>;
        static_assert(static_graph::value, "Invalid ECS stage graph");

        if constexpr (static_graph::size == 0) {
            return;
        } else if constexpr (static_graph::command_node_count == 0) {
            _call_stage_layers<static_graph>();
        } else {
            std::array<command_buffer, static_graph::size> cmdbufs{};
            _drive_stage_graph<static_graph>(
                cmdbufs, [this](auto& ready, size_t ready_count) {
                    for (size_t index = 0; index < ready_count; ++index) {
                        _stage_invokers<static_graph>::value[ready[index]](
                            this);
                    }
                });
        }
    }

    template <stage Stage, neutron::execution::scheduler Sch>
    void call(Sch& sch, std::span<command_buffer> cmdbufs) {
        using static_graph = stage_graph<Stage, Descriptor>;
        static_assert(static_graph::value, "Invalid ECS stage graph");

        if constexpr (static_graph::size == 0) {
            return;
        } else {
            if constexpr (static_graph::command_node_count != 0) {
                assert(cmdbufs.size() >= static_graph::size);
            }
            _drive_stage_graph<static_graph>(
                cmdbufs, [this, &sch](auto& ready, size_t ready_count) {
                    auto batch = execution::schedule(sch) |
                                 execution::bulk(
                                     std::execution::par,
                                     static_cast<uint32_t>(ready_count),
                                     [this, &ready](uint32_t batch_index) {
                                         _stage_invokers<static_graph>::value
                                             [ready[batch_index]](this);
                                     });
                    neutron::this_thread::sync_wait(std::move(batch));
                });
        }
    }

    template <stage Stage, neutron::execution::scheduler Sch, typename CmdBufs>
    void call(Sch& sch, CmdBufs& cmdbufs)
    requires requires(CmdBufs& buffers) {
        std::span<command_buffer>{ buffers };
        buffers.resize(size_t{});
    }
    {
        using static_graph = stage_graph<Stage, Descriptor>;
        static_assert(static_graph::value, "Invalid ECS stage graph");

        if constexpr (static_graph::command_node_count != 0) {
            if (cmdbufs.size() < static_graph::size) {
                cmdbufs.resize(static_graph::size);
            }
        }

        call<Stage>(sch, std::span<command_buffer>{ cmdbufs });
    }

private:
    // static constexpr auto _hash_array() noexcept {
    //     return neutron::make_hash_array<components>();
    // }

    // template <component Component>
    // static consteval index_t _static_index() {
    //     static_assert(std::same_as<Component,
    //     std::remove_cvref_t<Component>>);

    //     constexpr auto array = _hash_array();
    //     constexpr auto hash  = neutron::hash_of<Component>();
    //     auto it = std::lower_bound(array.begin(), array.end(), hash);
    //     if (it != array.end()) {
    //         return *it;
    //     }
    //     throw std::invalid_argument("Component not exists");
    // }

    template <size_t Index, auto Sys, typename T = decltype(Sys)>
    struct _call_sys;
    template <size_t Index, auto Sys, typename Ret, typename... Args>
    struct _call_sys<Index, Sys, Ret (*)(Args...)> {
        void operator()(basic_world* world) const {
            Sys(construct_from_world<Sys, Args, Index>(*world)...);
        }
    };
    template <size_t Index, auto Sys, typename Ret, typename... Args>
    struct _call_sys<Index, Sys, Ret (*const)(Args...)> {
        void operator()(basic_world* world) const {
            Sys(construct_from_world<Sys, Args, Index>(*world)...);
        }
    };
    template <size_t Index, auto Sys, typename Ret, typename... Args>
    struct _call_sys<Index, Sys, Ret (*)(Args...) noexcept> {
        void operator()(basic_world* world) const noexcept {
            Sys(construct_from_world<Sys, Args, Index>(*world)...);
        }
    };
    template <size_t Index, auto Sys, typename Ret, typename... Args>
    struct _call_sys<Index, Sys, Ret (*const)(Args...) noexcept> {
        void operator()(basic_world* world) const noexcept {
            Sys(construct_from_world<Sys, Args, Index>(*world)...);
        }
    };

    template <typename StaticGraph>
    struct _stage_invokers {
        using invoker_t = void (*)(basic_world*);

        template <size_t Index>
        static void invoke(basic_world* world) {
            using sysinfo_t = typename StaticGraph::template node_t<Index>;
            _call_sys<Index, sysinfo_t::fn>{}(world);
        }

        static constexpr auto value = []<size_t... Is>(
                                          std::index_sequence<Is...>) {
            return std::array<invoker_t, StaticGraph::size>{ &invoke<Is>... };
        }(std::make_index_sequence<StaticGraph::size>{});
    };

    template <typename StaticGraph, typename Layer>
    struct _layer_invokers;

    template <
        typename StaticGraph, template <typename...> typename Template,
        typename... SysInfos>
    struct _layer_invokers<StaticGraph, Template<SysInfos...>> {
        static void call(basic_world* world) {
            (_call_sys<
                 tuple_first_v<SysInfos, typename StaticGraph::traits_list>,
                 SysInfos::fn>{}(world),
             ...);
        }
    };

    template <typename StaticGraph, typename Layers>
    struct _stage_layers;

    template <
        typename StaticGraph, template <typename...> typename Template,
        typename... Layers>
    struct _stage_layers<StaticGraph, Template<Layers...>> {
        static void call(basic_world* world) {
            (_layer_invokers<StaticGraph, Layers>::call(world), ...);
        }
    };

    template <typename StaticGraph>
    void _call_stage_layers() {
        _stage_layers<StaticGraph, typename StaticGraph::layers>::call(this);
    }

    template <typename StaticGraph>
    void _prepare_command_buffers(std::span<command_buffer> cmdbufs) {
        command_buffers_ = cmdbufs;
        if constexpr (StaticGraph::command_node_count != 0) {
            assert(command_buffers_.size() >= StaticGraph::size);
        }
        _reset_command_buffers();
    }

    template <typename StaticGraph, typename ReadyExecutor>
    void _drive_stage_graph(
        std::span<command_buffer> cmdbufs, ReadyExecutor&& execute_ready) {
        _prepare_command_buffers<StaticGraph>(cmdbufs);

        stage_task_graph<StaticGraph> runtime_graph;
        std::array<size_t, StaticGraph::size> ready{};

        while (!runtime_graph.done()) {
            const auto ready_count = runtime_graph.collect_runnable(ready);
            if (ready_count == 0) {
                assert(runtime_graph.needs_flush());
                if (!runtime_graph.needs_flush()) {
                    break;
                }
                _flush_command_buffers();
                runtime_graph.flush();
                continue;
            }

            execute_ready(ready, ready_count);
            runtime_graph.complete(ready, ready_count);
        }

        if (runtime_graph.dirty_commands()) {
            _flush_command_buffers();
            runtime_graph.flush();
        }
    }

    void _reset_command_buffers() noexcept {
        for (auto& cmdbuf : command_buffers_) {
            cmdbuf.reset();
        }
    }

    void _flush_command_buffers() noexcept {
        for (auto& cmdbuf : command_buffers_) {
            cmdbuf.apply(_base());
            cmdbuf.reset();
        }
    }

    [[nodiscard]] double _update_interval() const noexcept {
        if constexpr (execute_traits::is_always) {
            return 0.0;
        } else if constexpr (execute_traits::has_dynamic_frequency) {
            return has_dynamic_update_interval_ ? dynamic_update_interval_
                                                : 0.0;
        } else if constexpr (execute_traits::has_frequency) {
            return execute_traits::frequency_interval;
        } else {
            return 0.0;
        }
    }

    /// variables could be use in only one specific system
    /// Locals are _sys_tuple, a tuple with system info, used to get the correct
    /// local for each sys
    type_list_rebind_t<neutron::shared_tuple, locals> locals_;
    type_list_rebind_t<neutron::shared_tuple, queries> queries_;
    //  variables could be pass between each systems
    type_list_rebind_t<neutron::shared_tuple, resources> resources_;

    std::span<command_buffer> command_buffers_{};
    const insertion_context* insertion_context_ = nullptr;
    typename clock_type::time_point last_update_{};
    double dynamic_update_interval_   = 0.0;
    bool has_dynamic_update_interval_ = false;
    bool has_last_update_             = false;
};

template <
    world_descriptor auto Descriptor,
    typename Alloc = std::allocator<std::byte>>
auto make_world(const Alloc& alloc = {})
    -> basic_world<decltype(Descriptor), Alloc> {
    return basic_world<decltype(Descriptor), Alloc>(alloc);
}

template <
    world_descriptor auto... Descriptors,
    typename Alloc = std::allocator<std::byte>>
auto make_worlds(const Alloc& alloc = {}) -> std::tuple<
    basic_world<decltype(Descriptors), rebind_alloc_t<Alloc, std::byte>>...> {
    return {
        basic_world<decltype(Descriptors), rebind_alloc_t<Alloc, std::byte>>(
            alloc)...
    };
}

namespace _basic_world {

struct _no_thread_slot {};

template <typename World>
using _descriptor_t = typename std::remove_cvref_t<World>::descriptor_type;

template <typename World>
using _execute_info_t = execute_info<_descriptor_t<World>>;

template <typename World>
inline constexpr bool _individual_world_v =
    _execute_info_t<World>::is_individual;

template <typename World>
inline constexpr size_t _group_index_v = _execute_info_t<World>::group_index;

template <typename... Worlds>
consteval size_t _max_group_index() {
    size_t result = 0;
    ((result =
          result < _group_index_v<Worlds> ? _group_index_v<Worlds> : result),
     ...);
    return result;
}

template <typename... Worlds>
inline constexpr size_t _max_group_index_v = _max_group_index<Worlds...>();

template <typename World>
using _thread_slot_t =
    std::conditional_t<_individual_world_v<World>, normthread, _no_thread_slot>;

template <typename... Worlds>
inline constexpr size_t _individual_world_count_v =
    (0U + ... + (_individual_world_v<Worlds> ? 1U : 0U));

struct _completion_state {
    explicit _completion_state(size_t count) noexcept
        : pending(static_cast<std::ptrdiff_t>(count)) {}

    void capture_exception(std::exception_ptr ex) noexcept {
        std::scoped_lock lock(mutex);
        if (!error) {
            error = std::move(ex);
        }
    }

    void wait() {
        pending.wait();
        if (error) {
            std::rethrow_exception(error);
        }
    }

    std::latch pending;
    std::mutex mutex;
    std::exception_ptr error{};
};

struct _count_down_receiver {
    using receiver_concept = execution::receiver_tag;

    _completion_state* state;

    void set_value() && noexcept { state->pending.count_down(); }

    template <typename Error>
    void set_error(Error&& err) && noexcept {
        if constexpr (std::same_as<
                          std::remove_cvref_t<Error>, std::exception_ptr>) {
            state->capture_exception(std::forward<Error>(err));
        } else {
            state->capture_exception(
                std::make_exception_ptr(std::forward<Error>(err)));
        }
        state->pending.count_down();
    }

    void set_stopped() && noexcept { state->pending.count_down(); }

    [[nodiscard]] auto get_env() const noexcept {
#if __has_include(<stdexec/execution.hpp>) || defined(ATOM_BUILTIN_EXECUTION) || __cpp_lib_execution >= 202902L
        return execution::env<>{};
#else
        return execution::empty_env{};
#endif
    }
};

struct _no_op_slot {};

template <typename Sender>
struct _connected_operation {
    using opstate_type = decltype(execution::connect(
        std::declval<Sender>(), std::declval<_count_down_receiver>()));

    template <typename Sndr>
    explicit _connected_operation(Sndr&& sndr, _completion_state& state)
        : opstate(
              execution::connect(
                  std::forward<Sndr>(sndr), _count_down_receiver{ &state })) {}

    void start() noexcept { execution::start(opstate); }

    opstate_type opstate;
};

template <stage Stage, typename World>
struct _call_stage_fn {
    World* world;

    void operator()() const { world->template call<Stage>(); }
};

template <typename World>
struct _call_update_fn {
    World* world;

    void operator()() const {
        if (world->should_call_update()) {
            world->template call<stage::pre_update>();
            world->template call<stage::update>();
            world->template call<stage::post_update>();
        }
    }
};

template <
    stage Stage, typename World, typename Thread,
    bool = _individual_world_v<World>>
struct _stage_op_slot {
    using scheduler_type = decltype(std::declval<Thread&>().get_scheduler());
    using sender_type =
        decltype(execution::schedule(std::declval<scheduler_type>()) | execution::then(std::declval<_call_stage_fn<Stage, World>>()));
    using type = std::optional<_connected_operation<sender_type>>;
};

template <stage Stage, typename World, typename Thread>
struct _stage_op_slot<Stage, World, Thread, false> {
    using type = _no_op_slot;
};

template <stage Stage, typename World, typename Thread>
using _stage_op_slot_t = typename _stage_op_slot<Stage, World, Thread>::type;

template <typename World, typename Thread, bool = _individual_world_v<World>>
struct _update_op_slot {
    using scheduler_type = decltype(std::declval<Thread&>().get_scheduler());
    using sender_type =
        decltype(execution::schedule(std::declval<scheduler_type>()) | execution::then(std::declval<_call_update_fn<World>>()));
    using type = std::optional<_connected_operation<sender_type>>;
};

template <typename World, typename Thread>
struct _update_op_slot<World, Thread, false> {
    using type = _no_op_slot;
};

template <typename World, typename Thread>
using _update_op_slot_t = typename _update_op_slot<World, Thread>::type;

template <
    stage Stage, size_t Index, typename WorldsTuple, typename ThreadTuple,
    typename OpTuple>
void _launch_individual_world(
    WorldsTuple& worlds, ThreadTuple& threads, OpTuple& ops,
    _completion_state& state) {
    auto& world   = std::get<Index>(worlds);
    auto& op      = std::get<Index>(ops);
    using world_t = std::remove_cvref_t<decltype(world)>;

    try {
        auto sender =
            execution::schedule(std::get<Index>(threads).get_scheduler()) |
            execution::then(_call_stage_fn<Stage, world_t>{ &world });
        op.emplace(std::move(sender), state);
        op->start();
    } catch (...) {
        state.capture_exception(std::current_exception());
        state.pending.count_down();
    }
}

template <
    size_t Index, typename WorldsTuple, typename ThreadTuple, typename OpTuple>
void _launch_individual_update(
    WorldsTuple& worlds, ThreadTuple& threads, OpTuple& ops,
    _completion_state& state) {
    auto& world   = std::get<Index>(worlds);
    auto& op      = std::get<Index>(ops);
    using world_t = std::remove_cvref_t<decltype(world)>;

    try {
        auto sender =
            execution::schedule(std::get<Index>(threads).get_scheduler()) |
            execution::then(_call_update_fn<world_t>{ &world });
        op.emplace(std::move(sender), state);
        op->start();
    } catch (...) {
        state.capture_exception(std::current_exception());
        state.pending.count_down();
    }
}

template <
    stage Stage, size_t GroupIndex, typename Sch, typename CmdBufs,
    typename... Worlds>
void _call_group_stage(
    Sch& sch, CmdBufs& cmdbufs, std::tuple<Worlds...>& worlds) {
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        (([&] {
             using world_t = std::tuple_element_t<Is, std::tuple<Worlds...>>;
             if constexpr (
                 !_individual_world_v<world_t> &&
                 _group_index_v<world_t> == GroupIndex) {
                 std::get<Is>(worlds).template call<Stage>(sch, cmdbufs);
             }
         }()),
         ...);
    }(std::index_sequence_for<Worlds...>{});
}

template <size_t GroupIndex, typename Sch, typename CmdBufs, typename... Worlds>
void _call_group_update(
    Sch& sch, CmdBufs& cmdbufs, std::tuple<Worlds...>& worlds) {
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        (([&] {
             using world_t = std::tuple_element_t<Is, std::tuple<Worlds...>>;
             if constexpr (
                 !_individual_world_v<world_t> &&
                 _group_index_v<world_t> == GroupIndex) {
                 auto& world = std::get<Is>(worlds);
                 if (world.should_call_update()) {
                     world.template call<stage::pre_update>(sch, cmdbufs);
                     world.template call<stage::update>(sch, cmdbufs);
                     world.template call<stage::post_update>(sch, cmdbufs);
                 }
             }
         }()),
         ...);
    }(std::index_sequence_for<Worlds...>{});
}

template <
    stage Stage, typename Sch, typename CmdBufs, typename ThreadTuple,
    typename... Worlds>
void _call_stage_all(
    Sch& sch, CmdBufs& cmdbufs, std::tuple<Worlds...>& worlds,
    ThreadTuple& threads) {
    if constexpr (sizeof...(Worlds) == 0) {
        return;
    } else {
        if constexpr (_individual_world_count_v<Worlds...> != 0) {
            auto ops = std::tuple<
                _stage_op_slot_t<Stage, Worlds, _thread_slot_t<Worlds>>...>{};
            _completion_state completion{
                _individual_world_count_v<Worlds...>
            };

            [&]<size_t... Is>(std::index_sequence<Is...>) {
                (void)std::initializer_list<int>{ (
                    [&] {
                        if constexpr (_individual_world_v<std::tuple_element_t<
                                          Is, std::tuple<Worlds...>>>) {
                            _launch_individual_world<Stage, Is>(
                                worlds, threads, ops, completion);
                        }
                    }(),
                    0)... };
            }(std::index_sequence_for<Worlds...>{});

            [&]<size_t... Groups>(std::index_sequence<Groups...>) {
                (_call_group_stage<Stage, Groups>(sch, cmdbufs, worlds), ...);
            }(std::make_index_sequence<_max_group_index_v<Worlds...> + 1>{});

            completion.wait();
        } else {
            [&]<size_t... Groups>(std::index_sequence<Groups...>) {
                (_call_group_stage<Stage, Groups>(sch, cmdbufs, worlds), ...);
            }(std::make_index_sequence<_max_group_index_v<Worlds...> + 1>{});
        }
    }
}

template <
    typename Sch, typename CmdBufs, typename ThreadTuple, typename... Worlds>
void _call_update_all(
    Sch& sch, CmdBufs& cmdbufs, std::tuple<Worlds...>& worlds,
    ThreadTuple& threads) {
    if constexpr (sizeof...(Worlds) == 0) {
        return;
    } else {
        if constexpr (_individual_world_count_v<Worlds...> != 0) {
            auto ops = std::tuple<
                _update_op_slot_t<Worlds, _thread_slot_t<Worlds>>...>{};
            _completion_state completion{
                _individual_world_count_v<Worlds...>
            };

            [&]<size_t... Is>(std::index_sequence<Is...>) {
                (void)std::initializer_list<int>{ (
                    [&] {
                        if constexpr (_individual_world_v<std::tuple_element_t<
                                          Is, std::tuple<Worlds...>>>) {
                            _launch_individual_update<Is>(
                                worlds, threads, ops, completion);
                        }
                    }(),
                    0)... };
            }(std::index_sequence_for<Worlds...>{});

            [&]<size_t... Groups>(std::index_sequence<Groups...>) {
                (_call_group_update<Groups>(sch, cmdbufs, worlds), ...);
            }(std::make_index_sequence<_max_group_index_v<Worlds...> + 1>{});

            completion.wait();
        } else {
            [&]<size_t... Groups>(std::index_sequence<Groups...>) {
                (_call_group_update<Groups>(sch, cmdbufs, worlds), ...);
            }(std::make_index_sequence<_max_group_index_v<Worlds...> + 1>{});
        }
    }
}

} // namespace _basic_world

template <stage Stage, world World>
void call(World& world) {
    world.template call<Stage>();
}

template <stage Stage, world... Worlds>
void call(std::tuple<Worlds...>& worlds) {
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        (std::get<Is>(worlds).template call<Stage>(), ...);
    }(std::index_sequence_for<Worlds...>());
}

template <
    stage Stage, neutron::execution::scheduler Sch, world World,
    typename CmdBufs>
void call(Sch& sch, CmdBufs& cmdbufs, World& world) {
    world.template call<Stage>(sch, cmdbufs);
}

template <
    stage Stage, neutron::execution::scheduler Sch, typename CmdBufs,
    world... Worlds>
void call(Sch& sch, CmdBufs& cmdbufs, std::tuple<Worlds...>& worlds) {
    if constexpr (sizeof...(Worlds) == 0) {
        return;
    } else {
        auto threads = std::tuple<_basic_world::_thread_slot_t<Worlds>...>{};
        _basic_world::_call_stage_all<Stage>(sch, cmdbufs, worlds, threads);
    }
}

void call_startup(world auto& world) {
    call<stage::pre_startup>(world);
    call<stage::startup>(world);
    call<stage::post_startup>(world);
}

template <world... Worlds>
void call_startup(std::tuple<Worlds...>& worlds) {
    call<stage::pre_startup>(worlds);
    call<stage::startup>(worlds);
    call<stage::post_startup>(worlds);
}

void call_startup(
    neutron::execution::scheduler auto& sch, auto& cmdbufs, world auto& world) {
    call<stage::pre_startup>(sch, cmdbufs, world);
    call<stage::startup>(sch, cmdbufs, world);
    call<stage::post_startup>(sch, cmdbufs, world);
}

template <world... Worlds>
void call_startup(
    neutron::execution::scheduler auto& sch, auto& cmdbufs,
    std::tuple<Worlds...>& worlds) {
    call<stage::pre_startup>(sch, cmdbufs, worlds);
    call<stage::startup>(sch, cmdbufs, worlds);
    call<stage::post_startup>(sch, cmdbufs, worlds);
}

void call_update(world auto& world) {
    if (!world.should_call_update()) {
        return;
    }
    call<stage::pre_update>(world);
    call<stage::update>(world);
    call<stage::post_update>(world);
}

template <world... Worlds>
void call_update(std::tuple<Worlds...>& worlds) {
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        (call_update(std::get<Is>(worlds)), ...);
    }(std::index_sequence_for<Worlds...>{});
}

void call_update(
    neutron::execution::scheduler auto& sch, auto& cmdbufs, world auto& world) {
    if (!world.should_call_update()) {
        return;
    }
    call<stage::pre_update>(sch, cmdbufs, world);
    call<stage::update>(sch, cmdbufs, world);
    call<stage::post_update>(sch, cmdbufs, world);
}

template <world... Worlds>
void call_update(
    neutron::execution::scheduler auto& sch, auto& cmdbufs,
    std::tuple<Worlds...>& worlds) {
    if constexpr (sizeof...(Worlds) == 0) {
        return;
    }

    auto threads = std::tuple<_basic_world::_thread_slot_t<Worlds>...>{};
    _basic_world::_call_update_all(sch, cmdbufs, worlds, threads);
}

} // namespace neutron
