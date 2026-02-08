#pragma once

namespace neutron::execution {

inline constexpr struct set_error_t {
    template <typename R, typename Error>
    void operator()(R&& rcvr, Error&& err) const noexcept {}
} set_error;

} // namespace neutron::execution
