#pragma once
#include <type_traits>
#include <utility> // IWYU pragma: keep, for initializer_list

namespace neutron {

template <typename Enum>
constexpr bool enum_anyof(Enum val, std::initializer_list<Enum> list) noexcept {
    using underlying_t = std::underlying_type_t<Enum>;
    underlying_t vals  = 0;
    for (auto val : list) {
        vals |= static_cast<underlying_t>(val);
    }
    return (static_cast<underlying_t>(val) & vals) != 0;
}

} // namespace neutron
