#pragma once
#include <utility>

namespace neutron::execution {

inline constexpr struct set_error_t {
    template <typename R, typename Error>
    void operator()(R&& rcvr, Error&& err) const noexcept {
        std::forward<R>(rcvr).set_error(std::forward<Error>(err));
    }
} set_error;

} // namespace neutron::execution
