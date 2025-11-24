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

// BUG: std::get is not SFINAE friendly
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

template <size_t Index>
constexpr inline _get::get_t<Index> get{};

template <size_t Index, typename Ty>
constexpr size_t gettible_size() noexcept {
    if constexpr (gettible<Ty, Index + 1>) {
        return gettible_size<Index + 1, Ty>();
    } else {
        return Index;
    }
}

} // namespace _get
/*! @endcond */

using _get::get;

using _get::gettible;

template <size_t Index, typename Ty>
using gettible_element_t = decltype(get<Index>(std::declval<Ty>()));

template <typename Ty>
constexpr size_t gettible_size =
    gettible<Ty, 0> ? _get::gettible_size<0, Ty>() : 0;

} // namespace neutron
