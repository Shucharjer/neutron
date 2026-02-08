#pragma once
#include "neutron/detail/execution/fwd.hpp"

namespace neutron::execution {

inline constexpr struct set_stopped_t {
    template <typename R>
    auto operator()(R&&) {}
} set_stopped;

} // namespace neutron::execution

