#pragma once
#include <cstdint>

namespace neutron {

inline constexpr struct get_max_concurrency_t {
    template <typename Desc>
    constexpr std::uint32_t operator()(Desc) const noexcept {
        return 1;
    }
} get_max_concurrency;

} // namespace neutron
