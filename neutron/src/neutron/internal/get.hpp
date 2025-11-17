#pragma once
#include <cstddef>

// for std::get
#include <array>
#include <tuple>
#include <utility>

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template <size_t Index>
struct _get {
    template <typename T>
    constexpr decltype(auto) operator()(T&& obj) const noexcept {
        if constexpr (requires { obj.template get<Index>(); }) {
            return obj.template get<Index>();
        } else if constexpr (requires { std::get<Index>(obj); }) {
            return std::get<Index>(obj);
        } else if constexpr (requires { get<Index>(obj); }) {
            return get<Index>(obj);
        }
    }
};

} // namespace internal
/*! @endcond */

template <size_t Index>
constexpr inline internal::_get<Index> get{};

template <size_t Index, typename Gettible>
concept gettible = requires(Gettible& obj) { get<Index>(obj); };

} // namespace neutron
