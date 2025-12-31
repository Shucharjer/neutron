#pragma once
#include "fwd.hpp"

#include <concepts>
#include <type_traits>
#include "./completion_signatures.hpp"
#include "./env.hpp"
#include "./receiver.hpp"

namespace neutron::execution {

struct sender_t {};

template <typename Sigs>
concept _valid_completion_signatures = true;

template <typename Sender>
concept enable_sender = std::derived_from<
    typename std::remove_cvref_t<Sender>::sender_concept, sender_t>;

template <typename Sender>
concept sender = enable_sender<Sender> && environment_provider<Sender> &&
                 std::move_constructible<Sender> &&
                 std::constructible_from<std::remove_cvref_t<Sender>, Sender>;

template <class Sndr, class Env = empty_env>
concept sender_in =
    sender<Sndr> && queryable<Env> && requires(Sndr&& sndr, Env&& env) {
        {
            get_completion_signatures(
                std::forward<Sndr>(sndr), std::forward<Env>(env))
        } -> _valid_completion_signatures;
    };

template <class Sndr, class Rcvr>
concept sender_to =
    sender_in<Sndr, env_of_t<Rcvr>> &&
    receiver_of<Rcvr, completion_signatures_of_t<Sndr, env_of_t<Rcvr>>> &&
    requires(Sndr&& sndr, Rcvr&& rcvr) {
        connect(std::forward<Sndr>(sndr), std::forward<Rcvr>(rcvr));
    };

} // namespace neutron::execution
