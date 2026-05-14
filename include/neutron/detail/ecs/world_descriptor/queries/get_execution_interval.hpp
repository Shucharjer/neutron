#pragma once
#include "neutron/detail/ecs/metainfo/execute.hpp"

namespace neutron {

inline constexpr struct get_execution_interval_t {
    template <typename Desc>
    constexpr double operator()(Desc) const noexcept {
        return _metainfo::execute_info<Desc>::frequency_interval;
    }
} get_execution_interval;

} // namespace neutron
