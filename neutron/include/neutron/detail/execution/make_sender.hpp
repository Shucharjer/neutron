#pragma once
#include <concepts>
#include <utility>
#include "neutron/detail/execution/fwd.hpp"

namespace neutron::execution {

template <typename Tag, typename Data, typename... Child>
struct _basic_sender {
    using sender_concept = sender_t;
    using _indices_for   = std::index_sequence_for<Child...>;

    decltype(auto) get_env() const noexcept;

    template <_decays_to<_basic_sender> Self, receiver Rcvr>
    auto connect(Self&& self, Rcvr rcvr) const;

    template <_decays_to<_basic_sender> Self, receiver Rcvr>
    auto connect(Self&& self, Rcvr rcvr);

    template <_decays_to<_basic_sender> Self, class Env>
    auto get_completion_signatures(Self&& self, Env&& env) noexcept;

private:
    template <_decays_to<_basic_sender> Self, receiver Rcvr>
    static auto _connect_impl(Self&& self, Rcvr rcvr);
};

template <typename Tag, typename Data, typename... Child>
requires std::semiregular<Tag> && std::movable<Data> && (sender<Child> && ...)
constexpr decltype(auto) make_sender(Tag tag, Data&& data, Child&&... child);

} // namespace neutron::execution
