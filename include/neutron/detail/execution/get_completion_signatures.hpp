#pragma once
#include <concepts>
#include <type_traits>
#include "neutron/detail/concepts/awaitable.hpp"
#include "neutron/detail/execution/default_domain.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/get_domain.hpp"

namespace neutron::execution {

inline constexpr struct get_completion_signatures_t {
    template <sender Sndr, typename Env>
    auto operator()(Sndr&& sndr, Env&& env) const noexcept {
        auto make = [](auto&& sender, auto&& environment) -> decltype(auto) {
            auto domain = _get_domain_late(sender, environment);
            return transform_sender(
                domain, std::forward<Sndr>(sender),
                std::forward<Env>(environment));
        };

        using sender_t    = std::remove_cvref_t<decltype(make(sndr, env))>;
        using decayed_env = std::remove_cvref_t<Env>;

        if constexpr (requires {
                          make(sndr, env).get_completion_signatures(env);
                      }) {
            return decltype(make(sndr, env).get_completion_signatures(env)){};
        } else if constexpr (requires {
                                 typename sender_t::completion_signatures;
                             }) {
            return typename sender_t::completion_signatures{};
        } else if constexpr (awaitable<sender_t, _env_promise<decayed_env>>) {
            using result_type =
                _await_result_type<sender_t, _env_promise<decayed_env>>;
            if constexpr (::std::same_as<void, result_type>) {
                return completion_signatures<
                    set_value_t(), set_error_t(::std::exception_ptr),
                    set_stopped_t()>{};
            } else {
                return completion_signatures<
                    set_value_t(result_type), set_error_t(::std::exception_ptr),
                    set_stopped_t()>{};
            }
        } else {
            static_assert(false);
        }
    }
} get_completion_signatures;

} // namespace neutron::execution
