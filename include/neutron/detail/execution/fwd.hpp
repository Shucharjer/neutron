#pragma once
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <thread> // IWYU pragma: keep
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include "neutron/detail/concepts/awaitable.hpp"
#include "neutron/detail/metafn/convert.hpp"
#include "neutron/detail/metafn/filt.hpp"
#include "neutron/detail/metafn/rebind.hpp"
#include "neutron/detail/metafn/size.hpp"
#include "neutron/detail/metafn/unique.hpp"
#include "neutron/detail/utility/fake_copy.hpp"
#include "neutron/detail/utility/get.hpp"

namespace neutron {

template <typename Ty, typename Promise>
concept _has_awaitable = // exposition only
    requires(Ty&& ty, Promise& promise) {
        { std::forward<Ty>(ty).as_awaitable(promise) } -> awaitable<Promise&>;
    };

template <typename Derived>
struct _with_await_transfrom {
    template <typename T>
    T&& await_transform(T&& value) noexcept {
        return std::forward<T>(value);
    }

    template <_has_awaitable<Derived> T>
    decltype(auto) await_transform(T&& value) noexcept(noexcept(
        std::forward<T>(value).as_awaitable(std::declval<Derived&>()))) {
        return std::forward<T>(value).as_awaitable(
            static_cast<Derived&>(*this));
    }
};

template <typename Env>
struct _env_promise : _with_await_transfrom<_env_promise<Env>> {
    auto get_return_object() noexcept;
    auto initial_suspend() noexcept;
    auto final_suspend() noexcept;
    void unhandled_exception() noexcept;
    void return_void() noexcept;
    std::coroutine_handle<> unhandled_stopped() noexcept;
    const Env& get_env() const noexcept;
};

template <typename Callable, typename... Args>
concept callable = std::is_invocable_v<Callable, Args...>;

struct never_stop_token {
    template <typename>
    using callback_type = void;

    constexpr bool stop_requested() const noexcept { return false; }
    constexpr bool stop_possible() const noexcept { return false; }
    friend constexpr bool
        operator==(never_stop_token, never_stop_token) noexcept = default;
};

// [exec.general], helper concepts
//   template<class T>
//     concept movable-value = see below; // exposition only

template <class From, class To>
concept _decays_to = std::same_as<std::decay_t<From>, To>; // exposition only

template <class T>
concept _class_type = _decays_to<T, T> && std::is_class_v<T>; // exposition only

// [exec.queryable], queryable objects
template <class T>
concept queryable = std::destructible<T>; // exposition only

// [exec.queries], queries
struct forwarding_query_t;
struct get_allocator_t;
struct get_stop_token_t;

extern const forwarding_query_t forwarding_query;
extern const get_allocator_t get_allocator;
extern const get_stop_token_t get_stop_token;

template <class T>
using stop_token_of_t =
    std::remove_cvref_t<decltype(get_stop_token(std::declval<T>()))>;

template <class T>
concept _forwarding_query = // exposition only
    forwarding_query(T{});
} // namespace neutron

namespace neutron::execution {

template <typename T, typename Promise>
using _await_result_type =
    decltype(_awaitable::_get_awaiter(
                 std::declval<T>(), std::declval<Promise&>())
                 .await_resume());

// [exec.queries], queries
enum class forward_progress_guarantee : uint8_t {
    concurrent,
    parallel,
    weakly_parallel
};
struct get_domain_t;
struct get_scheduler_t;
struct get_delegation_scheduler_t;
struct get_forward_progress_guarantee_t;
template <class CPO>
struct get_completion_scheduler_t;
struct get_await_completion_adaptor_t;

extern const get_domain_t get_domain;
extern const get_scheduler_t get_scheduler;
extern const get_delegation_scheduler_t get_delegation_scheduler;
extern const get_forward_progress_guarantee_t get_forward_progress_guarantee;
template <class CPO>
extern const get_completion_scheduler_t<CPO> get_completion_scheduler;
extern const get_await_completion_adaptor_t get_await_completion_adaptor;;

struct empty_env {};
struct get_env_t {
    template <typename EnvProvider>
    requires requires(const std::remove_cvref_t<EnvProvider>& ep) {
        { ep.get_env() } noexcept -> queryable;
    }
    auto operator()(EnvProvider&& ep) const noexcept {
        return std::as_const(ep).get_env();
    }

    template <typename EnvProvider>
    requires(!requires(const std::remove_cvref_t<EnvProvider>& ep) {
        { ep.get_env() } noexcept -> queryable;
    })
    auto operator()(EnvProvider&& ep) const noexcept {
        return empty_env{};
    }
};
inline constexpr get_env_t get_env;

template <class T>
using env_of_t = decltype(get_env(std::declval<T>()));

// [exec.domain.default], execution domains
struct default_domain;

// [exec.sched], schedulers
struct scheduler_t {};

// [exec.recv], receivers
struct receiver_t {};

struct set_value_t;
struct set_error_t;
struct set_stopped_t;

extern const set_value_t set_value;
extern const set_error_t set_error;
extern const set_stopped_t set_stopped;

// [exec.opstate], operation states
struct operation_state_t {};

struct start_t;
extern const start_t start;

template <class O>
concept operation_state =
    std::derived_from<typename O::operation_state_concept, operation_state_t> &&
    std::is_object_v<O> && requires(O& op) {
        { start(op) } noexcept;
    };

// [exec.snd], senders
struct sender_t {};

// [exec.getcomplsigs], completion signatures
struct get_completion_signatures_t;
extern const get_completion_signatures_t get_completion_signatures;

template <class Rcvr>
concept receiver =
    std::derived_from<
        typename std::remove_cvref_t<Rcvr>::receiver_concept, receiver_t> &&
    requires(const std::remove_cvref_t<Rcvr>& rcvr) {
        { get_env(rcvr) } -> queryable;
    } && std::move_constructible<std::remove_cvref_t<Rcvr>> &&
    std::constructible_from<std::remove_cvref_t<Rcvr>, Rcvr>;

template <class Signature, class Rcvr>
concept _valid_completion_for = // exposition only
    requires(Signature* sig) {
        []<class Tag, class... Args>(Tag (*)(Args...))
        requires callable<Tag, std::remove_cvref_t<Rcvr>, Args...>
        {}(sig);
    };

template <typename Fn>
constexpr bool _is_completion_signature = false;
template <typename... Vs>
constexpr bool _is_completion_signature<set_value_t(Vs...)> = true;
template <typename Err>
constexpr bool _is_completion_signature<set_error_t(Err)> = true;
template <>
constexpr bool _is_completion_signature<set_stopped_t()> = true;

template <typename Fn>
concept _completion_signature = _is_completion_signature<Fn>;

template <typename Tag, typename Ty>
struct _has_same_tag : std::false_type {};
template <typename Tag, typename... Args>
struct _has_same_tag<Tag, Tag(Args...)> : std::true_type {};

template <typename Tag, typename Sigs>
struct _cmplsigs_count_of {
    template <typename T>
    using _predicate = _has_same_tag<Tag, T>;
    static constexpr size_t value =
        type_list_size_v<type_list_filt_t<_predicate, Sigs>>;
};

template <_completion_signature... Fns>
struct completion_signatures {
    template <typename Tag>
    static constexpr size_t count_of(Tag) {
        return _cmplsigs_count_of<Tag, completion_signatures>::value;
    }

    template <typename Fn>
    static constexpr void for_each(Fn&& fn) {
        (fn(static_cast<Fns*>(nullptr)), ...);
    }
};

template <typename Sigs>
constexpr bool _is_valid_completion_signatures = false;
template <typename... Sigs>
constexpr bool _is_valid_completion_signatures<completion_signatures<Sigs...>> =
    true;

template <typename Ty>
concept _valid_completion_signatures = _is_valid_completion_signatures<Ty>;

template <class Rcvr, class Completions>
concept _has_completions = // exposition only
    requires(Completions* completions) {
        []<_valid_completion_for<Rcvr>... Sigs>(
            completion_signatures<Sigs...>*) {}(completions);
    };

template <class Rcvr, class Completions>
concept receiver_of = receiver<Rcvr> && _has_completions<Rcvr, Completions>;

template <class Sndr>
concept _is_sender = // exposition only
    std::derived_from<typename Sndr::sender_concept, sender_t>;

template <class Sndr>
concept _enable_sender =                      // exposition only
    _is_sender<Sndr> ||
    awaitable<Sndr, _env_promise<empty_env>>; // [exec.awaitables]

template <
    typename Tag, _valid_completion_signatures Completions,
    template <typename...> typename Tuple,
    template <typename...> typename Variant>
struct _gather_signatures_helper {
    template <typename Ty>
    using _predicate = _has_same_tag<Tag, Ty>;

    using _cmplsigs_t = type_list_filt_t<_predicate, Completions>;

    using _type_list = type_list_rebind_t<type_list, _cmplsigs_t>;

    template <typename>
    struct _convert;
    template <typename... Args>
    struct _convert<Tag(Args...)> {
        using type = Tuple<Args...>;
    };

    using _tuple_t = type_list_convert_t<_convert, _type_list>;

    using type = type_list_rebind_t<Variant, _tuple_t>;
};

template <
    typename Tag, _valid_completion_signatures Completions,
    template <typename...> typename Tuple,
    template <typename...> typename Variant>
using _gather_signatures =
    typename _gather_signatures_helper<Tag, Completions, Tuple, Variant>::type;

template <typename... Args>
using _decayed_tuple = std::tuple<std::decay_t<Args>...>;

struct _empty_variant {
    _empty_variant() = delete;
};

template <typename... T>
struct _variant_or_empty_helper {
    using type = unique_type_list_t<std::variant<T...>>;
};
template <>
struct _variant_or_empty_helper<> {
    using type = _empty_variant;
};

template <typename... Args>
using _variant_or_empty = typename _variant_or_empty_helper<Args...>::type;

template <class Sndr>
concept sender = _enable_sender<std::remove_cvref_t<Sndr>> &&
                 requires(const std::remove_cvref_t<Sndr>& sndr) {
                     { get_env(sndr) } -> queryable;
                 } &&
                 // senders are movable and decay copyable
                 std::move_constructible<std::remove_cvref_t<Sndr>> &&
                 std::constructible_from<std::remove_cvref_t<Sndr>, Sndr>;

template <class Sndr, class Env = empty_env>
concept sender_in =
    sender<Sndr> && queryable<Env> && requires(Sndr&& sndr, Env&& env) {
        {
            get_completion_signatures(
                std::forward<Sndr>(sndr), std::forward<Env>(env))
        } -> _valid_completion_signatures;
    };

template <class Sndr, class Env = empty_env>
requires sender_in<Sndr, Env>
using completion_signatures_of_t =
    std::invoke_result_t<get_completion_signatures_t, Sndr, Env>;

// [exec.connect], the connect sender algorithm
struct connect_t;
extern const connect_t connect;

// [exec.factories], sender factories
struct just_t;
struct just_error_t;
struct just_stopped_t;
struct schedule_t;

extern const just_t just;
extern const just_error_t just_error;
extern const just_stopped_t just_stopped;
extern const schedule_t schedule;
//   extern const  unspecified read;

template <class Sndr, class Rcvr>
concept sender_to =
    sender_in<Sndr, env_of_t<Rcvr>> &&
    receiver_of<Rcvr, completion_signatures_of_t<Sndr, env_of_t<Rcvr>>> &&
    requires(Sndr&& sndr, Rcvr&& rcvr) {
        connect(std::forward<Sndr>(sndr), std::forward<Rcvr>(rcvr));
    };

template <
    class Sndr, class Env = empty_env,
    template <class...> class Tuple   = _decayed_tuple,
    template <class...> class Variant = _variant_or_empty>
requires sender_in<Sndr, Env>
using value_types_of_t = _gather_signatures<
    set_value_t, completion_signatures_of_t<Sndr, Env>, Tuple, Variant>;

template <
    class Sndr, class Env = empty_env,
    template <class...> class Variant = _variant_or_empty>
requires sender_in<Sndr, Env>
using error_types_of_t = _gather_signatures<
    set_error_t, completion_signatures_of_t<Sndr, Env>, std::type_identity_t,
    Variant>;

template <
    typename Sndr, typename Env = empty_env,
    template <typename...> typename Tuple   = _decayed_tuple,
    template <typename...> typename Variant = _variant_or_empty>
requires sender_in<Sndr, Env>
constexpr bool sends_stopped = !std::same_as<
    type_list<>, _gather_signatures<
                     set_stopped_t, completion_signatures_of_t<Sndr, Env>,
                     type_list, type_list>>;

template <class Sch>
concept scheduler =
    std::derived_from<
        typename std::remove_cvref_t<Sch>::scheduler_concept, scheduler_t> &&
    queryable<Sch> &&
    requires(Sch&& sch) {
        { schedule(std::forward<Sch>(sch)) } -> sender;
        {
            get_completion_scheduler<set_value_t>(
                get_env(schedule(std::forward<Sch>(sch))))
        } -> std::same_as<std::remove_cvref_t<Sch>>;
    } && std::equality_comparable<std::remove_cvref_t<Sch>> &&
    std::copy_constructible<std::remove_cvref_t<Sch>>;

template <sender Sndr>
using tag_of_t = std::remove_pointer_t<decltype(fake_copy(
    std::forward<decltype(get<0>(std::declval<Sndr>()))>(
        get<0>(std::declval<Sndr>()))))>;

template <typename Sndr, typename Tag>
concept sender_for = sender<Sndr> && std::same_as<tag_of_t<Sndr>, Tag>;

template <class Sndr, class Rcvr>
using connect_result_t =
    decltype(connect(std::declval<Sndr>(), std::declval<Rcvr>()));

template <scheduler Sndr>
using schedule_result_t = decltype(schedule(std::declval<Sndr>()));

struct starts_on_t;
struct continues_on_t;
struct on_t;
struct schedule_from_t;
struct then_t;
struct upon_error_t;
struct upon_stopped_t;
struct let_value_t;
struct let_error_t;
struct let_stopped_t;
struct bulk_t;
struct split_t;
struct when_all_t;
struct when_all_with_variant_t;
struct into_variant_t;
struct stopped_as_optional_t;
struct stopped_as_error_t;

extern const starts_on_t starts_on;
extern const continues_on_t continues_on;
extern const on_t on;
extern const schedule_from_t schedule_from;
extern const then_t then;
extern const upon_error_t upon_error;
extern const upon_stopped_t upon_stopped;
extern const let_value_t let_value;
extern const let_error_t let_error;
extern const let_stopped_t let_stopped;
extern const bulk_t bulk;
extern const split_t split;
extern const when_all_t when_all;
extern const when_all_with_variant_t when_all_with_variant;
extern const into_variant_t into_variant;
extern const stopped_as_optional_t stopped_as_optional;
extern const stopped_as_error_t stopped_as_error;

// [exec.ctx], execution resources
// [exec.run.loop], run_loop
class run_loop;
} // namespace neutron::execution

namespace neutron::this_thread {

using namespace ::std::this_thread;

// [exec.consumers], consumers
struct sync_wait_t;
struct sync_wait_with_variant_t;

extern const sync_wait_t sync_wait;
extern const sync_wait_with_variant_t sync_wait_with_variant;

} // namespace neutron::this_thread

namespace neutron::execution {
// [exec.as.awaitable]
struct as_awaitable_t;
extern const as_awaitable_t as_awaitable;

// [exec.with.awaitable.senders]
template <_class_type Promise>
struct with_awaitable_senders;
} // namespace neutron::execution
