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
    #include "../src/neutron/internal/get.hpp"
    #include "../src/neutron/internal/pipeline.hpp"
    #include "../src/neutron/internal/tag_invoke.hpp"

namespace neutron::execution {

// basic concepts

template <typename Env>
concept queryable = std::destructible<Env>;

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

struct _none_such {};

enum class forward_progress_guarantee : uint8_t {
    concurrent,
    parallel,
    weakly_parallel
};

namespace queries {

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

constexpr inline _none_such _no_default{};

template <typename Derived, auto Default = _no_default>
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

struct get_domain_t {};

struct get_scheduler_t : _query<get_scheduler_t> {};

struct get_allocator_t : _query<get_allocator_t> {};

struct get_delegatee_scheduler_t : _query<get_delegatee_scheduler_t> {};

struct get_forward_progress_guarantee_t :
    _query<
        get_forward_progress_guarantee_t,
        forward_progress_guarantee::weakly_parallel> {};

template <typename CompletionTag>
struct get_completion_scheduler_t {
    template <typename Env>
    constexpr decltype(auto) operator()(Env&& env) const {
        return std::forward<Env>(env).get_completion_scheduler();
    }
};

template <typename Tag>
concept _completion_tag = one_of<Tag, set_value_t, set_error_t, set_stopped_t>;

struct get_stop_token_t : _query<get_stop_token_t> {};
} // namespace queries

using queries::get_domain_t;
using queries::get_scheduler_t;
using queries::get_delegatee_scheduler_t;
using queries::get_forward_progress_guarantee_t;
using queries::get_completion_scheduler_t;

constexpr inline get_domain_t get_domain{};
constexpr inline get_scheduler_t get_scheduler{};
constexpr inline get_delegatee_scheduler_t get_delegatee_scheduler{};
constexpr inline get_forward_progress_guarantee_t
    get_forward_progress_guarantee{};
template <typename Cpo>
constexpr inline get_completion_scheduler_t<Cpo> get_completion_scheduler{};

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
        return std::forward<Scheduler>(scheduler).schedule();
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

/// Let `sndr` be an expression such that `decltype((sndr))` is `Sndr`.
/// The type `tag_of_t<Sndr>` is as follows:
/// - If the declaration `auto&& [tag, data, ...children] = sndr;` would be
/// well-formed, `tag_of_t<Sndr>` is an alias for `decltype(auto(tag))`.
/// - Otherwise, `tag_of_t<Sndr>` is ill-formed.
template <sender Sndr>
requires gettible<Sndr, 0>
using tag_of_t = decltype(get<0>(std::declval<Sndr>()));

template <typename Sndr, typename... Args>
concept _has_member_transform_sender =
    sender<Sndr> && requires(Sndr&& sndr, Args&&... args) {
        tag_of_t<Sndr>().transform_sender(
            std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    };

template <typename Sndr, typename... Args>
concept _has_member_transform_env =
    sender<Sndr> && requires(Sndr&& sndr, Args&&... args) {
        tag_of_t<Sndr>().transform_env(
            std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    };

struct default_domain {
    template <sender Sndr, queryable... Env>
    requires(sizeof...(Env) <= 1)
    constexpr sender decltype(auto)
        transform_sender(Sndr&& sndr, const Env&... env) noexcept(
            _has_member_transform_sender<Sndr, const Env&...>
                ? noexcept(tag_of_t<Sndr>().transform_sender(
                      std::forward<Sndr>(sndr), env...))
                : true) {
        if constexpr (_has_member_transform_sender<Sndr, const Env&...>) {
            return tag_of_t<Sndr>().transform_sender(
                std::forward<Sndr>(sndr), env...);
        } else {
            return std::forward<Sndr>(sndr);
        }
    }

    template <sender Sndr, queryable Env>
    constexpr queryable decltype(auto)
        transform_env(Sndr&& sndr, Env&& env) noexcept(
            _has_member_transform_env<Sndr, Env&&>
                ? noexcept(tag_of_t<Sndr>().transform_env(
                      std::forward<Sndr>(sndr), std::forward<Env>(env)))
                : true) {
        if constexpr (_has_member_transform_env<Sndr, Env&&>) {
            return tag_of_t<Sndr>().transform_env(
                std::forward<Sndr>(sndr), std::forward<Env>(env));
        } else {
            return static_cast<Env>(std::forward<Env>(env));
        }
    }

    template <typename Tag, sender Sndr, typename... Args>
    static constexpr decltype(auto)
        apply_sender(Tag, Sndr&& sndr, Args&&... args) noexcept(
            noexcept(Tag().apply_sender(
                std::forward<Sndr>(sndr), std::forward<Args>(args)...))) {
        Tag().apply_sender(
            std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    }
};

template <typename Domain, sender Sndr, queryable... Env>
requires(sizeof...(Env) <= 1)
constexpr sender decltype(auto)
    transform_sender(Domain dom, Sndr&& sndr, Env&&... env) noexcept {
    if constexpr (requires {
                      dom.transform_sender(
                          std::forward<Sndr>(sndr), std::forward<Env>(env)...);
                  }) {
        return dom.transform_sender(
            std::forward<Sndr>(sndr), std::forward<Env>(env)...);
    } else {
        return default_domain().transform_sender(
            std::forward<Sndr>(sndr), std::forward<Env>(env)...);
    }
}

template <typename Domain, sender Sndr, queryable Env>
constexpr queryable decltype(auto)
    transform_env(Domain dom, Sndr&& sndr, Env&& env) noexcept {
    if constexpr (requires {
                      dom.transform_env(
                          std::forward<Sndr>(sndr), std::forward<Env>(env));
                  }) {
        return dom.transform_env(
            std::forward<Sndr>(sndr), std::forward<Env>(env));
    } else {
        return default_domain().transform_env(
            std::forward<Sndr>(sndr), std::forward<Env>(env));
    }
}

template <typename Domain, typename... Args>
concept _has_apply_sender = requires(Domain dom, Args&&... args) {
    dom.apply_sender(std::forward<Args>(args)...);
};

template <typename Domain, typename Tag, sender Sndr, typename... Args>
constexpr decltype(auto) apply_sender(
    Domain dom, Tag, Sndr&& sndr,
    Args&&... args) noexcept(_has_apply_sender<Domain, Tag, Sndr, Args...>) {
    if constexpr (_has_apply_sender<Domain, Tag, Sndr, Args...>) {
        return dom.apply_sender(
            Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    } else {
        return default_domain().apply_sender(
            Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    }
}

template <typename Derived>
struct sender_adaptor_closure : adaptor_closure<Derived> {};

template <typename Ty>
concept _sender_adaptor_closure = _adaptor_closure<Ty, sender_adaptor_closure>;

template <typename Ty, typename Sender>
concept _sender_adaptor_closure_for =
    sender<std::remove_cvref_t<Sender>> &&
    _adaptor_closure_for<Ty, sender_adaptor_closure, Sender> &&
    sender<std::invoke_result_t<Ty, std::remove_cvref_t<Ty>>>;

template <typename Closure1, typename Closure2>
struct _sender_closure_compose :
    public _closure_compose<Closure1, Closure2, sender_adaptor_closure> {
    using _compose_base =
        _closure_compose<Closure1, Closure2, sender_adaptor_closure>;

    using _compose_base::_compose_base;
    using _compose_base::operator();
};

template <typename C1, typename C2>
_sender_closure_compose(C1&&, C2&&) -> _sender_closure_compose<
    std::remove_cvref_t<C1>, std::remove_cvref_t<C2>>;

// policy

constexpr inline struct just_t {
    using sender_concept = sender_t;
    template <typename Value>
    constexpr auto operator()(Value&& value) const;
} just{};

constexpr inline struct transfer_just_t {
    using sender_concept = sender_t;
    template <scheduler Scheduler, typename... Args>
    constexpr auto operator()(Scheduler&& scheduler, Args&&... args)
        -> decltype(auto) {
        auto domain = query_or(get_domain, scheduler, default_domain{});
        return transform_sender(domain, std::forward<Scheduler>(scheduler));
    }
} transfer_just;

constexpr inline struct sync_wait_t {

} sync_wait{};

template <typename Tag, typename Data, typename... Children>
class _sender {
public:
    using sender_concept = sender_t;

private:
};

template <typename Tag, typename Data, typename... Children>
constexpr auto make_sender(Tag tag, Data&& data, Children&&... children) {
    return _sender(
        tag, std::forward<Data>(data), std::forward<Children>(children)...);
}

namespace sender_adaptors {

struct on_t {
    template <scheduler Scheduler, sender Sndr>
    constexpr auto operator()(Scheduler&& sch, Sndr&& sndr) const {
        auto domain = _get_early_domain(sndr);
        return transform_sender(
            domain,
            make_sender(
                *this, std::forward<Scheduler>(sch), std::forward<Sndr>(sndr)));
    }

    template <
        sender Sndr, scheduler Scheduler,
        _sender_adaptor_closure_for<Sndr> Closure>
    constexpr auto
        operator()(Sndr&& sndr, Scheduler&& sch, Closure&& closure) const {
        // TODO:
    }
};
struct transfer_t {};
struct schedule_from_t {};
struct then_t {};
struct upon_error_t {};
struct upon_stopped_t {};
struct let_value_t {};
struct let_error_t {};
struct let_stopped_t {};
struct bulk_t {};
struct split_t {};
struct when_all_t {};
struct when_all_with_variant_t {};
struct into_variant_t {};
struct stopped_as_optional_t {};
struct stopped_as_error_t {};
struct ensure_started_t {};

} // namespace sender_adaptors

using sender_adaptors::on_t;
using sender_adaptors::transfer_t;
using sender_adaptors::schedule_from_t;
using sender_adaptors::then_t;
using sender_adaptors::upon_error_t;
using sender_adaptors::upon_stopped_t;
using sender_adaptors::let_value_t;
using sender_adaptors::let_error_t;
using sender_adaptors::let_stopped_t;
using sender_adaptors::bulk_t;
using sender_adaptors::split_t;
using sender_adaptors::when_all_t;
using sender_adaptors::when_all_with_variant_t;
using sender_adaptors::into_variant_t;
using sender_adaptors::stopped_as_optional_t;
using sender_adaptors::stopped_as_error_t;
using sender_adaptors::ensure_started_t;

inline constexpr on_t on{};
inline constexpr transfer_t transfer{};
inline constexpr schedule_from_t schedule_from{};
inline constexpr then_t then{};
inline constexpr upon_error_t upon_error{};
inline constexpr upon_stopped_t upon_stopped{};
inline constexpr let_value_t let_value{};
inline constexpr let_error_t let_error{};
inline constexpr let_stopped_t let_stopped{};
inline constexpr bulk_t bulk{};
inline constexpr split_t split{};
inline constexpr when_all_t when_all{};
inline constexpr when_all_with_variant_t when_all_with_variant{};
inline constexpr into_variant_t into_variant{};
inline constexpr stopped_as_optional_t stopped_as_optional;
inline constexpr stopped_as_error_t stopped_as_error;
inline constexpr ensure_started_t ensure_started{};

namespace sender_consumers {

struct start_detached_t {};

} // namespace sender_consumers

using sender_consumers::start_detached_t;

inline constexpr start_detached_t start_detached{};

} // namespace neutron::execution

template <
    neutron::execution::sender Sender,
    neutron::execution::_sender_adaptor_closure_for<Sender> Closure>
constexpr decltype(auto) operator|(Sender&& sender, Closure&& closure) noexcept(
    std::is_nothrow_invocable_v<Closure, Sender>) {
    return std::forward<Closure>(closure)(std::forward<Sender>(sender));
}

template <
    neutron::execution::_sender_adaptor_closure C1,
    neutron::execution::_sender_adaptor_closure C2>
constexpr auto operator|(C1&& closure1, C2&& closure2)
    -> neutron::execution::_sender_closure_compose<
        std::remove_cvref_t<C1>, std::remove_cvref_t<C2>> {
    return _sender_closure_compose(
        std::forward<C1>(closure1), std::forward<C2>(closure2));
}

#endif
