#pragma once
#include <concepts>
#include "./env.hpp"

namespace neutron::execution {

struct receiver_t {};

template <typename Receiver>
concept enable_receiver = std::derived_from<
    typename std::remove_cvref_t<Receiver>::receiver_concept, receiver_t>;

template <typename Receiver>
concept receiver =
    enable_receiver<Receiver> && environment_provider<Receiver> &&
    std::move_constructible<Receiver> &&
    std::constructible_from<std::remove_cvref_t<Receiver>, Receiver>;

template <typename Rcvr, typename Completions>
concept has_completions = true;

template <typename Receiver, typename Completions>
concept receiver_of =
    receiver<Receiver> && has_completions<Receiver, Completions>;

}