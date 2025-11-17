#pragma once
#include <concepts>
#include <cstdlib>
#include <type_traits>
#include "neutron/template_list.hpp"

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

struct poly_empty_impl {
    struct _universal {
        template <typename Ty>
        [[noreturn]] constexpr operator Ty&&() {
            // If you meet error here, you must called function in static vtable
            // without initializing it by an polymorphic object.
            std::abort();
        }
    };

    template <size_t Index, typename Object = void, typename... Args>
    constexpr auto invoke(Args&&...) noexcept {
        return _universal{};
    }
    template <size_t Index, typename Object = void, typename... Args>
    constexpr auto invoke(Args&&...) const noexcept {
        return _universal{};
    }
};

template <typename Object>
concept has_interface =
    requires { typename Object::template interface<poly_empty_impl>; } &&
    std::derived_from<
        typename Object::template interface<poly_empty_impl>, poly_empty_impl>;

template <typename Object>
concept implementable =
    requires {
        typename Object::template impl<
            typename Object::template interface<poly_empty_impl>>;
    } &&
    is_value_list_v<typename Object::template impl<
        typename Object::template interface<poly_empty_impl>>>;

} // namespace internal
/*! @endcond */

template <typename Object>
concept basic_poly_object =
    internal::has_interface<Object> && internal::implementable<Object>;

template <typename Object>
concept poly_object = basic_poly_object<Object>;

template <typename Impl, typename Object>
concept poly_impl = poly_object<Object> && requires {
    typename Object::template impl<std::remove_cvref_t<Impl>>;
} && !requires { typename Impl::poly_tag; };

} // namespace neutron
