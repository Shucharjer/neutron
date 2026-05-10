#pragma once
#include <cstdint>
#include <type_traits>
#include "neutron/detail/ecs/metainfo/graph.hpp"

namespace neutron {

/**
 * @brief Returns the maximum number of command buffers needed by a world.
 */
inline constexpr struct get_max_buffer_count_t {
    template <typename Desc>
    consteval std::uint32_t operator()(Desc) const noexcept {
        return _metainfo::descriptor_graph<
            std::remove_cvref_t<Desc>>::max_buffer_count;
    }
} get_max_buffer_count;

} // namespace neutron
