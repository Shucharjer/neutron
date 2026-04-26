#pragma once
#include "neutron/detail/macros.hpp"
#include "neutron/detail/stop_token/fwd.hpp"

namespace neutron {

class inplace_stop_token {
public:
    template <class CallbackFn>
    using callback_type = inplace_stop_callback<CallbackFn>;

    inplace_stop_token()                             = default;
    bool operator==(const inplace_stop_token&) const = default;

    // [stoptoken.inplace.mem], member functions
    ATOM_NODISCARD bool stop_requested() const noexcept;
    ATOM_NODISCARD bool stop_possible() const noexcept;
    void swap(inplace_stop_token&) noexcept;

private:
    const inplace_stop_source* stop_source_ = nullptr; // exposition only
};

} // namespace neutron
