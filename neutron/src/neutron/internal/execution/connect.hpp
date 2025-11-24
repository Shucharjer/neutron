#pragma once
#include "./fwd.hpp"

namespace neutron::execution {

constexpr inline struct connect_t {
    template <typename Sndr, typename Rcvr>
    auto operator()(Sndr&& sndr, Rcvr&& rcvr) const;
} connect;

} // namespace neutron::execution
