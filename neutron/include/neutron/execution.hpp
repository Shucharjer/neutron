#pragma once

#include <cstdint>
#include <utility>
#include <version>
#if defined(__cpp_lib_execution) && __cpp_lib_execution >= 202602L

    #include <execution>

namespace neutron::execution {

using namespace std::execution;

}

#elif __has_include(<stdexec/execution>)

    #include <stdexec/execution>

namespace neutron::execution {

using namespace stdexec;

}

#else

    #include <concepts>
    #include <type_traits>
    #include "../src/neutron/internal/concepts/one_of.hpp"
    #include "../src/neutron/internal/tag_invoke.hpp"

namespace neutron::execution {

// basic concepts

template <typename Env>
concept queryable = std::destructible<Env>;

template <typename Env, typename Query, typename... Args>
concept _member_queryable_with =
    requires(const Env& env, const Query& query, Args... args) {
        { env.query(query, args...) };
    };

template <typename Env, typename Query, typename... Args>
concept _nothrow_member_queryable_with =
    _member_queryable_with<Env, Query, Args...> &&
    requires(const Env& env, const Query& query, Args... args) {
        { env.query(query, args...) } noexcept;
    };

template <typename Derived>
struct _query { // NOLINT(bugprone-crtp-constructor-accessibility)
    template <typename Env, typename... Args>
    requires _member_queryable_with<Env, Derived, Args...>
    constexpr auto
        operator()(const Env& env, const Derived& query, Args&&... args) noexcept(
            _nothrow_member_queryable_with<Env, Derived, Args...>) {
        return env.query(query, std::forward<Args>(args)...);
    }

    template <typename Env, typename... Args>
    requires(!_member_queryable_with<Env, Derived, Args...>) &&
            tag_invocable<Derived, const Env&, Args...>
    constexpr auto
        operator()(const Env& env, const Derived& query, Args&&... args) noexcept(
            nothrow_tag_invocable<Derived, const Env&, Args...>) {
        return tag_invoke(query, env, std::forward<Args>(args)...);
    }
};

constexpr inline struct connect_t {
    template <typename Sndr, typename Rcvr>
    constexpr auto operator()(Sndr&& sndr, Rcvr&& rcvr) const;
} connect{};

template <typename EnvProvider>
concept _has_member_get_env = requires(EnvProvider& ep) { ep.get_env(); };

constexpr inline struct get_env_t {
    template <typename EnvProvider>
    requires _has_member_get_env<EnvProvider>
    constexpr auto operator()(const EnvProvider& ep) const {
        return ep.get_env();
    }

    template <typename EnvProvider>
    requires(!_has_member_get_env<EnvProvider>) &&
            tag_invocable<get_env_t, EnvProvider>
    constexpr auto operator()(const EnvProvider& ep) const {
        return tag_invoke(get_env_t{}, ep);
    }
} get_env{};

template <typename Rcvr>
using env_of_t = std::invoke_result_t<get_env_t, Rcvr>;

struct empty_env {};

template <typename CompSigs>
concept valid_completion_signatures = true;

constexpr inline struct get_completion_signatures_t {
    template <typename Sndr, typename Env>
    constexpr auto operator()(Sndr&& sndr, Env&& env) const;
} get_completion_signatures{};

template <typename Sndr, typename Env>
using completion_signatures_of_t =
    std::invoke_result_t<get_completion_signatures_t, Sndr, Env>;

constexpr inline struct get_allocator_t : _query<get_allocator_t> {
} get_allocator;

constexpr inline struct get_domain_t {

} get_domain;

constexpr inline struct get_scheduler_t {
    template <typename Env>
    constexpr auto operator()(Env& env) const {
        return env.get_scheduler();
    }
} get_scheduler;

constexpr inline struct set_value_t {

    template <typename Rcvr, typename... Args>
    constexpr static bool _has_member_set_value = requires {
        {
            std::declval<Rcvr>().set_value(std::declval<Args>()...)
        } noexcept -> std::same_as<void>;
    };

    template <typename Rcvr, typename... Args>
    requires _has_member_set_value<Rcvr, Args...>
    constexpr void operator()(Rcvr&& rcvr, Args&&... args) const noexcept {
        std::forward<Rcvr>(rcvr).set_value(std::forward<Args>(args)...);
    }

    template <typename Rcvr, typename... Args>
    requires(!_has_member_set_value<Rcvr, Args...>) &&
            nothrow_tag_invocable<set_value_t, Rcvr, Args...>
    constexpr void operator()(Rcvr&& rcvr, Args&&... args) const noexcept {
        tag_invoke(
            set_value_t{}, std::forward<Rcvr>(rcvr),
            std::forward<Args>(args)...);
    }
} set_value;

constexpr inline struct set_error_t {

    template <typename Rcvr, typename Err>
    constexpr static bool _has_member_set_error = requires {
        {
            std::declval<Rcvr>().set_error(std::declval<Err>())
        } noexcept -> std::same_as<void>;
    };

    template <typename Rcvr, typename Err>
    requires _has_member_set_error<Rcvr, Err>
    constexpr void operator()(Rcvr&& rcvr, Err&& err) const noexcept {
        std::forward<Rcvr>(rcvr).set_error(std::forward<Err>(err));
    }

    template <typename Rcvr, typename Err>
    requires(!_has_member_set_error<Rcvr, Err>) &&
            nothrow_tag_invocable<set_error_t, Rcvr, Err>
    constexpr void operator()(Rcvr&& rcvr, Err&& err) const noexcept {
        tag_invoke(
            set_error_t{}, std::forward<Rcvr>(rcvr), std::forward<Err>(err));
    }
} set_error;

constexpr inline struct set_stopped_t {
    template <typename Rcvr>
    constexpr static bool _has_member_set_stopped = requires {
        { std::declval<Rcvr>().set_stopped() } noexcept -> std::same_as<void>;
    };

    template <typename Rcvr>
    requires _has_member_set_stopped<Rcvr>
    constexpr decltype(auto) operator()(Rcvr&& rcvr) const noexcept {
        std::forward<Rcvr>(rcvr).set_stopped();
    }

    template <typename Rcvr>
    requires(!_has_member_set_stopped<Rcvr>) &&
            nothrow_tag_invocable<set_stopped_t, Rcvr>
    constexpr decltype(auto) operator()(Rcvr&& rcvr) const noexcept {
        tag_invoke(set_stopped_t{}, std::forward<Rcvr>(rcvr));
    }
} set_stopped;

template <typename CompletionTag>
struct get_completion_scheduler_t {
    template <typename Env>
    constexpr decltype(auto) operator()(Env&& env) const {
        return env.get_completion_scheduler();
    }
};

template <typename Tag>
concept _completion_tag = one_of<Tag, set_value_t, set_error_t, set_stopped_t>;

template <_completion_tag CompletionTag>
constexpr inline get_completion_scheduler_t<CompletionTag>
    get_completion_scheduler{};

struct receiver_t {};

template <typename Receiver>
concept enable_receiver = std::derived_from<
    typename std::remove_cvref_t<Receiver>::receiver_concept, receiver_t>;

template <typename Receiver>
concept _env_provider = requires(const std::remove_cvref_t<Receiver>& rcvr) {
    { get_env(rcvr) } -> queryable;
};

template <typename Receiver>
concept receiver =
    enable_receiver<Receiver> && _env_provider<Receiver> &&
    std::move_constructible<Receiver> &&
    std::constructible_from<std::remove_cvref_t<Receiver>, Receiver>;

template <typename Rcvr, typename Completions>
concept has_completions = true;

template <typename Receiver, typename Completions>
concept receiver_of =
    receiver<Receiver> && has_completions<Receiver, Completions>;

struct sender_t {};

template <typename Sender>
concept enable_sender = std::derived_from<
    typename std::remove_cvref_t<Sender>::sender_concept, sender_t>;

template <typename Sender>
concept sender = enable_sender<Sender> &&
                 requires(const std::remove_cvref_t<Sender>& sndr) {
                     { get_env(sndr) } -> queryable;
                 } && std::move_constructible<Sender> &&
                 std::constructible_from<std::remove_cvref_t<Sender>, Sender>;

template <class Sndr, class Env = empty_env>
concept sender_in =
    sender<Sndr> && queryable<Env> && requires(Sndr&& sndr, Env&& env) {
        {
            get_completion_signatures(
                std::forward<Sndr>(sndr), std::forward<Env>(env))
        } -> valid_completion_signatures;
    };

template <class Sndr, class Rcvr>
concept sender_to =
    sender_in<Sndr, env_of_t<Rcvr>> &&
    receiver_of<Rcvr, completion_signatures_of_t<Sndr, env_of_t<Rcvr>>> &&
    requires(Sndr&& sndr, Rcvr&& rcvr) {
        connect(std::forward<Sndr>(sndr), std::forward<Rcvr>(rcvr));
    };

struct scheduler_t {};

template <typename Scheduler>
concept _enable_scheduler = std::derived_from<
    typename std::remove_cvref_t<Scheduler>::scheduler_concept, scheduler_t>;

constexpr inline struct schedule_t {
    template <typename Scheduler>
    constexpr decltype(auto) operator()(Scheduler&& scheduler) const {
        return scheduler.schedule();
    }
} schedule;

template <typename Ty>
constexpr auto _decay_copy(Ty) -> Ty;

template <typename Scheduler>
concept scheduler = _enable_scheduler<Scheduler> && queryable<Scheduler> &&
                    requires(Scheduler&& sch) {
                        { schedule(std::forward<Scheduler>(sch)) } -> sender;
                        {
                            _decay_copy(
                                get_completion_scheduler<set_value_t>(get_env(
                                    schedule(std::forward<Scheduler>(sch)))))
                        } -> std::same_as<std::remove_cvref_t<Scheduler>>;
                    };

enum class forward_progress_guarantee : uint8_t {
    concurrent,
    parallel,
    weakly_parallel
};

constexpr inline struct get_forward_progress_guarantee_t :
    _query<get_forward_progress_guarantee_t> {
} get_forward_progress_guarantee;

struct default_domain {};

template <sender Sender1, sender Sender2>
constexpr inline decltype(auto)
    operator|(Sender1&& sender1, Sender2&& sender2) noexcept {
    // TODO:
}

// policy

constexpr inline struct just_t {
    template <typename Value>
    constexpr auto operator()(Value&& value) const;
} just{};

constexpr inline struct transfer_just_t {
    template <scheduler Scheduler, typename... Args>
    constexpr auto operator()(Scheduler&& scheduler, Args&&... args)
        -> decltype(auto) {
        auto domain = query_or(get_domain, scheduler, default_domain{});
        return transform_sender(domain, std::forward<Scheduler>(scheduler));
    }
} transfer_just;

constexpr inline struct then_t {
    template <typename Fn>
    constexpr auto operator()(Fn&& fn) const;
} then{};

constexpr inline struct on_t {
} on{};

constexpr inline struct when_all_t {

} when_all{};

constexpr inline struct sync_wait_t {

} sync_wait{};

} // namespace neutron::execution

#endif
