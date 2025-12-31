#pragma once
#include <concepts>
#include <tuple>
#include <utility>
#include "receiver.hpp"
#include "./sender.hpp"

namespace neutron::execution {

template <typename Tag, typename Data, typename... Child>
class basic_sender : std::tuple<Tag, Data, Child...> {
public:
    using sender_concept     = sender_t;
    using _indices_for_child = std::index_sequence_for<Child...>;
    using _base              = std::tuple<Tag, Data, Child...>;
    using _base::_base;

    constexpr decltype(auto) get_env() const noexcept {}

    constexpr auto get_completion_signatures() const noexcept {}

    template <receiver Receiver>
    constexpr auto connect(Receiver receiver) & {}

    template <receiver Receiver>
    constexpr auto connect(Receiver receiver) const& {}

    template <receiver Receiver>
    constexpr auto connect(Receiver receiver) && {}

private:
};

template <std::semiregular Tag, typename Data, typename... Child>
constexpr auto _make_sender(Tag tag, Data&& data, Child&&... child) {
    return basic_sender<Tag, std::decay_t<Data>, std::decay_t<Child>...>(
        tag, std::forward<Data>(data), std::forward<Child>(child)...);
}

} // namespace neutron::execution
