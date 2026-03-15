#pragma once
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

    constexpr world_schedule_context(
        Sch& sch, CmdBufs& cmdbufs, std::tuple<Worlds...>& worlds) noexcept
        : sch_(sch), cmdbufs_(cmdbufs), worlds_(worlds) {}

    template <stage Stage, size_t Group = 0>
    void call_group() {
        _basic_world::_call_group_stage<Stage, Group>(sch_, cmdbufs_, worlds_);
    }

    template <size_t Group = 0>
    void call_update_group() {
        _basic_world::_call_group_update<Group>(sch_, cmdbufs_, worlds_);
    }

    [[nodiscard]] auto& scheduler() noexcept { return sch_; }
    [[nodiscard]] auto& command_buffers() noexcept { return cmdbufs_; }
    [[nodiscard]] auto& worlds() noexcept { return worlds_; }

private:
    Sch& sch_;
    CmdBufs& cmdbufs_;
    std::tuple<Worlds...>& worlds_;
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
inline constexpr bool _is_render_enabled_v =
    render_info<_descriptor_t<World>>::is_enabled;

template <world World>
inline constexpr bool _has_render_stage_v =
    stage_graph<stage::render, _descriptor_t<World>>::size != 0;

template <world... Worlds>
inline constexpr size_t _render_enabled_count_v =
    (0U + ... + (_is_render_enabled_v<Worlds> ? 1U : 0U));

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

} // namespace _world_schedule

template <typename Hooks, typename Sch, typename CmdBufs, world... Worlds>
class basic_world_schedule :
    public world_schedule_context<Sch, CmdBufs, Worlds...>,
    private Hooks {
    using _context_base = world_schedule_context<Sch, CmdBufs, Worlds...>;

    static_assert(
        _world_schedule::_render_enabled_count_v<Worlds...> == 1,
        "basic_world_schedule requires exactly one world marked with "
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

    void startup() {
        call_startup(
            this->scheduler(), this->command_buffers(), this->worlds());
        call<stage::first>(
            this->scheduler(), this->command_buffers(), this->worlds());
    }

    [[nodiscard]] bool step() {
        if (!_world_schedule::_poll_events(hooks(), *this)) [[unlikely]] {
            return true;
        }
        if (_world_schedule::_is_stopped(hooks(), *this)) [[unlikely]] {
            return false;
        }

        call_update(this->scheduler(), this->command_buffers(), this->worlds());
        _world_schedule::_render_begin(hooks(), *this);
        call<stage::render>(
            this->scheduler(), this->command_buffers(), this->worlds());
        _world_schedule::_render_end(hooks(), *this);
        return true;
    }

    void shutdown() {
        call<stage::last>(
            this->scheduler(), this->command_buffers(), this->worlds());
        call<stage::shutdown>(
            this->scheduler(), this->command_buffers(), this->worlds());
    }

    void run() {
        startup();
        while (step()) {}
        shutdown();
    }
};

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
