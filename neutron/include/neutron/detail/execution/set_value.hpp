#pragma once
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron::execution {

inline constexpr struct set_value_t {
    template <typename R, typename... Args>
    ATOM_FORCE_INLINE auto operator()(R&& rcvr, Args&&... args) const {
        std::forward<R>(rcvr).set_value(std::forward<Args>(args)...);
    }
} set_value;

} // namespace neutron::execution
