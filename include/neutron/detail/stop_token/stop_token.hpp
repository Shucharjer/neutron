#pragma once
#include "neutron/detail/stop_token/fwd.hpp"

namespace neutron {

class never_stop_token {
    struct _callback_type {
        explicit _callback_type(never_stop_token, auto&&) noexcept {}
    };

public:
    template <typename>
    using callback_type = _callback_type;

    static constexpr bool stop_requested() noexcept { return false; }
    static constexpr bool stop_possible() noexcept { return false; }

    bool operator==(const never_stop_token&) const = default;
};

} // namespace neutron
