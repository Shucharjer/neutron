#pragma once
#include <utility>

namespace neutron::execution {

template <typename Tag, typename Env, typename Value>
requires requires(const Tag& tag, const Env& env) { tag(env); }
constexpr auto _query_with_default(Tag, const Env& env, Value&&) noexcept(
    noexcept(Tag()(env))) -> decltype(auto) {
    return Tag()(env);
}

template <typename Tag, typename Env, typename Value>
constexpr auto _query_with_default(Tag, const Env&, Value&& value) noexcept(
    noexcept(static_cast<Value>(std::forward<Value>(value))))
    -> decltype(auto) {
    return static_cast<Value>(std::forward<Value>(value));
}

} // namespace neutron::execution
