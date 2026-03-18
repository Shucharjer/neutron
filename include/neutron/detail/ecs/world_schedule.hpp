#pragma once
#include <array>
#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>
#include "neutron/detail/ecs/world.hpp"
#include "neutron/execution.hpp"

namespace neutron {

template <typename Sch, typename CmdBufs, world... Worlds>
class world_schedule_context {
public:
    using scheduler_type       = Sch;
    using command_buffers_type = CmdBufs;
    using worlds_type          = std::tuple<Worlds...>;
    using thread_slots_type =
        std::tuple<_basic_world::_thread_slot_t<Worlds>...>;

    constexpr world_schedule_context(
        Sch& sch, CmdBufs& cmdbufs, std::tuple<Worlds...>& worlds) noexcept
        : sch_(sch), cmdbufs_(cmdbufs), worlds_(worlds) {}

    template <stage Stage>
    void call() {
        _basic_world::_call_stage_all<Stage>(sch_, cmdbufs_, worlds_, threads_);
    }

    template <stage Stage, size_t Group = 0>
    void call_group() {
        _basic_world::_call_group_stage<Stage, Group>(sch_, cmdbufs_, worlds_);
    }

    void call_update() {
        _basic_world::_call_update_all(sch_, cmdbufs_, worlds_, threads_);
    }

    template <size_t Group = 0>
    void call_update_group() {
        _basic_world::_call_group_update<Group>(sch_, cmdbufs_, worlds_);
    }

    [[nodiscard]] auto& scheduler() noexcept { return sch_; }
    [[nodiscard]] auto& command_buffers() noexcept { return cmdbufs_; }
    [[nodiscard]] auto& worlds() noexcept { return worlds_; }
    [[nodiscard]] auto& thread_slots() noexcept { return threads_; }

private:
    Sch& sch_;
    CmdBufs& cmdbufs_;
    std::tuple<Worlds...>& worlds_;
    thread_slots_type threads_{};
};

namespace _world_schedule {

template <typename Fn>
struct _hook_t {
    Fn fn;

    template <typename Context>
    constexpr void operator()(Context& context) const {
        if constexpr (std::invocable<const Fn&, Context&>) {
            fn(context);
        } else {
            fn();
        }
    }
};

template <stage Stage, size_t Group>
struct _call_group_t {};

template <size_t Group>
struct _call_update_group_t {};

template <typename Context, typename Fn>
constexpr void _apply(Context& context, const _hook_t<Fn>& hook) {
    hook(context);
}

template <typename Context, stage Stage, size_t Group>
constexpr void _apply(Context& context, _call_group_t<Stage, Group>) {
    context.template call_group<Stage, Group>();
}

template <typename Context, size_t Group>
constexpr void _apply(Context& context, _call_update_group_t<Group>) {
    context.template call_update_group<Group>();
}

template <world World>
using _descriptor_t = typename std::remove_cvref_t<World>::descriptor_type;

template <world World>
inline constexpr bool _is_events_enabled_v =
    events_info<_descriptor_t<World>>::is_enabled;

template <world World>
inline constexpr bool _is_render_enabled_v =
    render_info<_descriptor_t<World>>::is_enabled;

template <world World>
inline constexpr bool _has_events_stage_v =
    stage_graph<stage::events, _descriptor_t<World>>::size != 0;

template <world World>
inline constexpr bool _has_render_stage_v =
    stage_graph<stage::render, _descriptor_t<World>>::size != 0;

template <world... Worlds>
inline constexpr size_t _events_enabled_count_v =
    (0U + ... + (_is_events_enabled_v<Worlds> ? 1U : 0U));

template <world... Worlds>
inline constexpr size_t _render_enabled_count_v =
    (0U + ... + (_is_render_enabled_v<Worlds> ? 1U : 0U));

template <world... Worlds>
inline constexpr bool _events_stage_ownership_v =
    (... && (_is_events_enabled_v<Worlds> || !_has_events_stage_v<Worlds>));

template <world... Worlds>
inline constexpr bool _render_stage_ownership_v =
    (... && (_is_render_enabled_v<Worlds> || !_has_render_stage_v<Worlds>));

template <typename Hooks, typename Runtime>
concept _poll_events_with_runtime = requires(Hooks& hooks, Runtime& runtime) {
    { hooks.poll_events(runtime) } -> std::convertible_to<bool>;
};

template <typename Hooks>
concept _poll_events_plain = requires(Hooks& hooks) {
    { hooks.poll_events() } -> std::convertible_to<bool>;
};

template <typename Hooks, typename Runtime>
concept _is_stopped_with_runtime = requires(Hooks& hooks, Runtime& runtime) {
    { hooks.is_stopped(runtime) } -> std::convertible_to<bool>;
};

template <typename Hooks>
concept _is_stopped_plain = requires(Hooks& hooks) {
    { hooks.is_stopped() } -> std::convertible_to<bool>;
};

template <typename Hooks, typename Runtime>
concept _render_begin_with_runtime =
    requires(Hooks& hooks, Runtime& runtime) { hooks.render_begin(runtime); };

template <typename Hooks>
concept _render_begin_plain = requires(Hooks& hooks) { hooks.render_begin(); };

template <typename Hooks, typename Runtime>
concept _render_end_with_runtime =
    requires(Hooks& hooks, Runtime& runtime) { hooks.render_end(runtime); };

template <typename Hooks>
concept _render_end_plain = requires(Hooks& hooks) { hooks.render_end(); };

template <typename Hooks, typename Runtime>
constexpr bool _poll_events(Hooks& hooks, Runtime& runtime) {
    if constexpr (_poll_events_with_runtime<Hooks, Runtime>) {
        return hooks.poll_events(runtime);
    } else if constexpr (_poll_events_plain<Hooks>) {
        return hooks.poll_events();
    } else {
        return true;
    }
}

template <typename Hooks, typename Runtime>
constexpr bool _is_stopped(Hooks& hooks, Runtime& runtime) {
    if constexpr (_is_stopped_with_runtime<Hooks, Runtime>) {
        return hooks.is_stopped(runtime);
    } else if constexpr (_is_stopped_plain<Hooks>) {
        return hooks.is_stopped();
    } else {
        return false;
    }
}

template <typename Hooks, typename Runtime>
constexpr void _render_begin(Hooks& hooks, Runtime& runtime) {
    if constexpr (_render_begin_with_runtime<Hooks, Runtime>) {
        hooks.render_begin(runtime);
    } else if constexpr (_render_begin_plain<Hooks>) {
        hooks.render_begin();
    }
}

template <typename Hooks, typename Runtime>
constexpr void _render_end(Hooks& hooks, Runtime& runtime) {
    if constexpr (_render_end_with_runtime<Hooks, Runtime>) {
        hooks.render_end(runtime);
    } else if constexpr (_render_end_plain<Hooks>) {
        hooks.render_end();
    }
}

template <typename WorldsTuple>
class _scoped_insertion_context;

template <world... Worlds>
class _scoped_insertion_context<std::tuple<Worlds...>> {
    using _worlds_tuple = std::tuple<Worlds...>;

public:
    explicit _scoped_insertion_context(
        _worlds_tuple& worlds, insertion_context context) noexcept
        : worlds_(&worlds),
          context_(context),
          previous_(_capture(std::index_sequence_for<Worlds...>{})) {
        _bind(std::index_sequence_for<Worlds...>{});
    }

    _scoped_insertion_context(const _scoped_insertion_context&) = delete;
    _scoped_insertion_context& operator=(const _scoped_insertion_context&) =
        delete;

    ~_scoped_insertion_context() noexcept {
        _restore(std::index_sequence_for<Worlds...>{});
    }

private:
    template <size_t... Is>
    auto _capture(std::index_sequence<Is...>) noexcept {
        return std::array<const insertion_context*, sizeof...(Worlds)>{
            world_accessor::insertion_context(std::get<Is>(*worlds_))...
        };
    }

    template <size_t... Is>
    void _bind(std::index_sequence<Is...>) noexcept {
        ((
             world_accessor::insertion_context(std::get<Is>(*worlds_)) =
                 &context_),
         ...);
    }

    template <size_t... Is>
    void _restore(std::index_sequence<Is...>) noexcept {
        ((
             world_accessor::insertion_context(std::get<Is>(*worlds_)) =
                 previous_[Is]),
         ...);
    }

    _worlds_tuple* worlds_;
    insertion_context context_{};
    std::array<const insertion_context*, sizeof...(Worlds)> previous_;
};

template <world... Worlds>
auto bind_insertions(
    std::tuple<Worlds...>& worlds, insertion_context context) noexcept
    -> _scoped_insertion_context<std::tuple<Worlds...>> {
    return _scoped_insertion_context<std::tuple<Worlds...>>{
        worlds, std::move(context)
    };
}

} // namespace _world_schedule

template <typename Hooks, typename Sch, typename CmdBufs, world... Worlds>
class basic_world_schedule :
    public world_schedule_context<Sch, CmdBufs, Worlds...>,
    private Hooks {
    using _context_base = world_schedule_context<Sch, CmdBufs, Worlds...>;

    static_assert(
        _world_schedule::_events_enabled_count_v<Worlds...> <= 1,
        "basic_world_schedule allows at most one world marked with "
        "enable_events");
    static_assert(
        _world_schedule::_events_stage_ownership_v<Worlds...>,
        "only the world marked with enable_events may contain events-stage "
        "systems");
    static_assert(
        _world_schedule::_render_enabled_count_v<Worlds...> <= 1,
        "basic_world_schedule allows at most one world marked with "
        "enable_render");
    static_assert(
        _world_schedule::_render_stage_ownership_v<Worlds...>,
        "only the world marked with enable_render may contain render-stage "
        "systems");

public:
    using hooks_type = Hooks;

    template <typename HookArg>
    constexpr basic_world_schedule(
        HookArg&& hooks, Sch& sch, CmdBufs& cmdbufs,
        std::tuple<Worlds...>&
            worlds) noexcept(std::is_nothrow_constructible_v<Hooks, HookArg>)
        : _context_base(sch, cmdbufs, worlds),
          Hooks(std::forward<HookArg>(hooks)) {}

    [[nodiscard]] auto& hooks() noexcept {
        return static_cast<hooks_type&>(*this);
    }

    [[nodiscard]] const auto& hooks() const noexcept {
        return static_cast<const hooks_type&>(*this);
    }

private:
    void _startup_impl() {
        this->template call<stage::pre_startup>();
        this->template call<stage::startup>();
        this->template call<stage::post_startup>();
        this->template call<stage::first>();
    }

    [[nodiscard]] bool _step_impl() {
        if (!_world_schedule::_poll_events(hooks(), *this)) [[unlikely]] {
            return !_world_schedule::_is_stopped(hooks(), *this);
        }

        this->template call<stage::events>();
        if (_world_schedule::_is_stopped(hooks(), *this)) [[unlikely]] {
            return false;
        }

        this->call_update();
        if constexpr (_world_schedule::_render_enabled_count_v<Worlds...> != 0) {
            _world_schedule::_render_begin(hooks(), *this);
            this->template call<stage::render>();
            _world_schedule::_render_end(hooks(), *this);
        }
        return true;
    }

    void _shutdown_impl() {
        this->template call<stage::last>();
        this->template call<stage::shutdown>();
    }

    template <typename Fn, typename... Insertions>
    decltype(auto) _with_insertions(Fn&& fn, Insertions&&... insertions) {
        auto bindings = make_insertion_bindings(
            hooks(), std::forward<Insertions>(insertions)...);
        auto scoped =
            _world_schedule::bind_insertions(this->worlds(), bindings.context());
        return std::forward<Fn>(fn)();
    }

public:
    void startup() {
        _with_insertions([this] { _startup_impl(); });
    }

    template <typename... Insertions>
    void startup(Insertions&&... insertions) {
        _with_insertions(
            [this] { _startup_impl(); }, std::forward<Insertions>(insertions)...);
    }

    [[nodiscard]] bool step() {
        return _with_insertions([this] { return _step_impl(); });
    }

    template <typename... Insertions>
    [[nodiscard]] bool step(Insertions&&... insertions) {
        return _with_insertions(
            [this] { return _step_impl(); },
            std::forward<Insertions>(insertions)...);
    }

    void shutdown() {
        _with_insertions([this] { _shutdown_impl(); });
    }

    template <typename... Insertions>
    void shutdown(Insertions&&... insertions) {
        _with_insertions(
            [this] { _shutdown_impl(); },
            std::forward<Insertions>(insertions)...);
    }

    void run() {
        _with_insertions([this] {
            _startup_impl();
            while (_step_impl()) {}
            _shutdown_impl();
        });
    }

    template <typename... Insertions>
    void run(Insertions&&... insertions) {
        _with_insertions(
            [this] {
                _startup_impl();
                while (_step_impl()) {}
                _shutdown_impl();
            },
            std::forward<Insertions>(insertions)...);
    }
};

template <typename Hooks, typename Sch, typename CmdBufs, world... Worlds>
using basic_runtime = basic_world_schedule<Hooks, Sch, CmdBufs, Worlds...>;

template <
    ::neutron::execution::scheduler Sch, typename CmdBufs, world... Worlds>
auto make_world_schedule(
    Sch& sch, CmdBufs& cmdbufs, std::tuple<Worlds...>& worlds)
    -> world_schedule_context<Sch, CmdBufs, Worlds...> {
    return { sch, cmdbufs, worlds };
}

template <
    typename Hooks, execution::scheduler Sch, typename CmdBufs, world... Worlds>
auto make_world_schedule(
    Hooks&& hooks, Sch& sch, CmdBufs& cmdbufs, std::tuple<Worlds...>& worlds)
    -> basic_world_schedule<
        std::remove_cvref_t<Hooks>, Sch, CmdBufs, Worlds...> {
    return { std::forward<Hooks>(hooks), sch, cmdbufs, worlds };
}

template <
    ::neutron::execution::scheduler Sch, typename CmdBufs, world... Worlds>
auto make_runtime(
    Sch& sch, CmdBufs& cmdbufs, std::tuple<Worlds...>& worlds)
    -> world_schedule_context<Sch, CmdBufs, Worlds...> {
    return make_world_schedule(sch, cmdbufs, worlds);
}

template <
    typename Hooks, execution::scheduler Sch, typename CmdBufs, world... Worlds>
auto make_runtime(
    Hooks&& hooks, Sch& sch, CmdBufs& cmdbufs, std::tuple<Worlds...>& worlds)
    -> basic_runtime<std::remove_cvref_t<Hooks>, Sch, CmdBufs, Worlds...> {
    return { std::forward<Hooks>(hooks), sch, cmdbufs, worlds };
}

template <typename Fn>
constexpr auto hook(Fn&& fn) {
    return _world_schedule::_hook_t<std::remove_cvref_t<Fn>>{ std::forward<Fn>(
        fn) };
}

template <stage Stage, size_t Group = 0>
inline constexpr _world_schedule::_call_group_t<Stage, Group> call_group{};

template <size_t Group = 0>
inline constexpr _world_schedule::_call_update_group_t<Group>
    call_update_group{};

template <typename Context, typename... Ops>
constexpr void drive(Context& context, Ops&&... ops) {
    (_world_schedule::_apply(context, std::forward<Ops>(ops)), ...);
}

} // namespace neutron
