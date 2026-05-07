#pragma once
#include "neutron/detail/ecs/metainfo/execute.hpp"

namespace neutron {

inline constexpr struct get_execution_policy_t {
    template <typename Desc>
    constexpr auto operator()(Desc) const noexcept {
        return typename _metainfo::execute_info<Desc>::execution_policy{};
    }
} get_execution_policy;

} // namespace neutron
