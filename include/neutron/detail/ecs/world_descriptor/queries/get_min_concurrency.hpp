#pragma once
#include <cstdint>
#include <type_traits>
#include "neutron/detail/ecs/metainfo/graph.hpp"

namespace neutron {

/**
 * @brief Returns the minimum worker thread count needed by a timely world.
 */
inline constexpr struct get_min_concurrency_t {
    template <typename Desc>
    consteval std::uint32_t operator()(Desc) const noexcept {
        return _metainfo::descriptor_graph<
            std::remove_cvref_t<Desc>>::min_concurrency;
    }
} get_min_concurrency;

} // namespace neutron
