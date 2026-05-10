#pragma once
#include <cstdint>
#include <type_traits>
#include "neutron/detail/ecs/metainfo/graph.hpp"

namespace neutron {

/**
 * @brief Returns the minimum command buffer count needed by a timely world.
 */
inline constexpr struct get_min_buffer_count_t {
    template <typename Desc>
    consteval std::uint32_t operator()(Desc) const noexcept {
        return _metainfo::descriptor_graph<
            std::remove_cvref_t<Desc>>::min_buffer_count;
    }
} get_min_buffer_count;

} // namespace neutron
