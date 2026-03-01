#pragma once
#include "neutron/detail/execution/fwd.hpp"

namespace neutron::execution {

class inline_scheduler {
    class _inline_sender;

    template <receiver Rcvr>
    class _inline_state;

public:
    using scheduler_concept = scheduler_t;
    constexpr _inline_sender schedule() noexcept;
    constexpr bool operator==(const inline_scheduler&) const noexcept = default;
};

} // namespace neutron::execution
