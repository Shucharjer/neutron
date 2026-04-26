#pragma once
#include <utility>
#include "neutron/detail/execution/fwd.hpp"

namespace neutron::execution {

inline constexpr struct set_stopped_t {
    template <typename R>
    auto operator()(R&& rcvr) const noexcept {
        std::forward<R>(rcvr).set_stopped();
    }
} set_stopped;

} // namespace neutron::execution
