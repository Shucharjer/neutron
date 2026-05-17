#pragma once
#include "neutron/detail/execution/core.hpp"

namespace neutron::execution {

inline constexpr struct start_detached_t {

    template <typename Sndr>
    struct state {};

    template <typename Sndr>
    struct receiver {
        using receiver_concept = receiver_tag;

        template <typename Error>
        void set_error(Error&& error) && noexcept {}
    };

    template <typename Sndr>
    requires sender_to<Sndr, receiver<Sndr>>
    constexpr void apply_sender(Sndr&& sndr) const {
        state<Sndr> state;
        auto op = connect(sndr, receiver<Sndr>{ &state });
        start(op);
    }

    template <sender Sndr>
    constexpr void operator()(Sndr&& sndr) const {
        auto dom = get_domain(sndr);
        return apply_sender(dom, *this, std::forward<Sndr>(sndr));
    }
} start_detached;

} // namespace neutron::execution
