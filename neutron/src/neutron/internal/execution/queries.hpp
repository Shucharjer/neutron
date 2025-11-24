#pragma once
#include "./fwd.hpp"

#include <utility>
#include "../concepts/one_of.hpp"
#include "../tag_invoke.hpp"

namespace neutron::execution {

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

struct _none_such {};

constexpr inline _none_such _no_default{};

template <typename Derived, auto Default = _no_default>
struct _query { // NOLINT(bugprone-crtp-constructor-accessibility)
    template <typename Env, typename... Args>
    requires _member_queryable_with<Env, Derived, Args...>
    constexpr auto operator()(const Env& env, Args&&... args) const
        noexcept(_nothrow_member_queryable_with<Env, Derived, Args...>) {
        return env.query(Derived{}, std::forward<Args>(args)...);
    }

    template <typename Env, typename... Args>
    requires(!_member_queryable_with<Env, Derived, Args...>) &&
            tag_invocable<Derived, const Env&, Args...>
    constexpr auto
        operator()(const Env& env, const Derived& query, Args&&... args) const
        noexcept(nothrow_tag_invocable<Derived, const Env&, Args...>) {
        return tag_invoke(Derived{}, env, std::forward<Args>(args)...);
    }
};

struct get_domain_t {};

struct get_scheduler_t : _query<get_scheduler_t> {};

struct get_allocator_t : _query<get_allocator_t> {};

struct get_delegation_scheduler_t : _query<get_delegation_scheduler_t> {};

struct get_forward_progress_guarantee_t :
    _query<
        get_forward_progress_guarantee_t,
        forward_progress_guarantee::weakly_parallel> {};

template <typename CompletionTag>
struct get_completion_scheduler_t :
    _query<get_completion_scheduler_t<CompletionTag>> {};

template <typename Tag>
concept _completion_tag = one_of<Tag, set_value_t, set_error_t, set_stopped_t>;

struct get_stop_token_t : _query<get_stop_token_t> {};

} // namespace queries

using queries::get_domain_t;
using queries::get_scheduler_t;
using queries::get_delegation_scheduler_t;
using queries::get_forward_progress_guarantee_t;
using queries::get_completion_scheduler_t;

constexpr inline get_domain_t get_domain{};
constexpr inline get_scheduler_t get_scheduler{};
constexpr inline get_delegation_scheduler_t get_delegatee_scheduler{};
constexpr inline get_forward_progress_guarantee_t
    get_forward_progress_guarantee{};
template <typename Cpo>
constexpr inline get_completion_scheduler_t<Cpo> get_completion_scheduler{};

} // namespace neutron::execution
