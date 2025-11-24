#pragma once
#include <utility>
#include "../get.hpp"
#include "./sender.hpp"

namespace neutron::execution {

template <typename Tag, typename Data, typename... Child>
class basic_sender : product_type<Tag, Data, Child...> {
public:
    using sender_concept = sender_t;
    using _indices_for   = std::index_sequence_for<Child...>;

    constexpr decltype(auto) get_env() const noexcept {
        auto& [_, data, ... child] = *this;
        return _impls_for<Tag>::_get_attrs(data, child...);
    }

    template <size_t Index>
    constexpr decltype(auto) get() && {
        return ::neutron::get<Index>(std::move(children_));
    }

    template <size_t Index>
    constexpr decltype(auto) get() const& {
        return ::neutron::get<Index>(children_);
    }

private:
    std::tuple<Child...> children_;
};

template <typename Tag, sender Sndr>
constexpr sender auto _make_sender(Sndr&& sndr) noexcept {
    return basic_sender<Tag, Sndr>(std::forward<Sndr>(sndr));
}

} // namespace neutron::execution
