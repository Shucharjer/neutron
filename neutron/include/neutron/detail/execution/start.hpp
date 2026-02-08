#pragma once

namespace neutron::execution {

inline constexpr struct start_t {
    template <typename OpState>
    void operator()(OpState& opstate) const noexcept {
        opstate.start();
    }
} start{};

} // namespace neutron::execution
