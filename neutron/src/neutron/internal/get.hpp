#pragma once
#include <cstddef>

// for std::get
#include <array>   // IWYU pragma: keep
#include <tuple>   // IWYU pragma: keep
#include <utility> // IWYU pragma: keep

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */
namespace _get {

template <typename T, size_t Index>
concept _has_member_get =
    requires(T&& obj) { std::forward<T>(obj).template get<Index>(); };

template <typename T, size_t Index>
concept _has_adl_get = requires(T&& obj) { get<Index>(std::forward<T>(obj)); };

template <typename T, size_t Index>
concept _has_std_get =
    requires(T&& obj) { std::get<Index>(std::forward<T>(obj)); };

template <typename T, size_t Index>
concept gettible = _has_member_get<T, Index> || _has_std_get<T, Index> ||
                   _has_adl_get<T, Index>;

template <typename T, size_t Index>
concept _has_nothrow_member_get = requires(T&& obj) {
    { std::forward<T>(obj).template get<Index>() } noexcept;
};

template <typename T, size_t Index>
concept _has_nothrow_adl_get = requires(T&& obj) {
    { get<Index>(std::forward<T>(obj)) } noexcept;
};

template <typename T, size_t Index>
concept _has_nothrow_std_get = requires(T&& obj) {
    { std::get<Index>(std::forward<T>(obj)) } noexcept;
};

template <typename T, size_t Index>
concept nothrow_gettible =
    _has_nothrow_member_get<T, Index> || _has_nothrow_adl_get<T, Index> ||
    _has_nothrow_std_get<T, Index>;

template <size_t Index>
struct get_t {
    template <gettible<Index> T>
    requires(!nothrow_gettible<T, Index>)
    constexpr decltype(auto) operator()(T&& obj) const {
        if constexpr (_has_member_get<T, Index>) {
            return std::forward<T>(obj).template get<Index>();
        } else if constexpr (_has_adl_get<T, Index>) {
            return get<Index>(obj);
        } else if constexpr (_has_std_get<T, Index>) {
            return std::get<Index>(obj);
        }
    }

    template <nothrow_gettible<Index> T>
    constexpr decltype(auto) operator()(T&& obj) const noexcept {
        if constexpr (_has_nothrow_member_get<T, Index>) {
            return std::forward<T>(obj).template get<Index>();
        } else if constexpr (_has_nothrow_adl_get<T, Index>) {
            return get<Index>(obj);
        } else if constexpr (_has_nothrow_std_get<T, Index>) {
            return std::get<Index>(obj);
        }
    }
};

} // namespace _get
/*! @endcond */

template <size_t Index>
constexpr inline _get::get_t<Index> get{};

using _get::gettible;

} // namespace neutron
