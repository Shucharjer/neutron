// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once

#ifndef __has_feature
    #define __has_feature(x) 0
#endif

#if __has_feature(__cpp_trivial_relocatability) &&                             \
    __cpp_trivial_relocatability >= 202502L && false

namespace neutron {

template <typename Ty>
using trivially_relocatable = std::is_trivially_relocatable_v<Ty>; // the prossible interface

}

#else
    #include "neutron/detail/macros.hpp"

    #if ATOM_HAS_REFLECTION

        #include <concepts>
        #include <meta>
        #include <type_traits>

namespace neutron {

namespace _trivially_relocatable {

template <typename Ty>
struct _is_trivially_relocatable;

template <typename Ty>
constexpr bool _has_any_virtual_base_class = [] consteval {
    using namespace std::meta;
    auto ctx = access_context::current();
    for (auto info : std::define_static_array(bases_of(^^Ty, ctx))) {
        if (is_virtual(info)) {
            return true;
        }
    }
    return false;
}();

template <typename Ty>
constexpr bool _has_not_trivially_relocatable_base_class = [] consteval {
    using namespace std::meta;
    constexpr auto ctx = access_context::current();
    template for (constexpr auto info :
                  std::define_static_array(bases_of(^^Ty, ctx))) {
        if (!_is_trivially_relocatable<typename[:type_of(info):]>::value) {
            return true;
        }
    }
    return false;
}();

template <typename Ty>
constexpr bool _has_not_trivially_relocatable_nonstatic_data_member =
    [] consteval {
        using namespace std::meta;
        constexpr auto ctx = access_context::current();
        template for (constexpr auto info : std::define_static_array(
                          nonstatic_data_members_of(^^Ty, ctx))) {
            if (!_is_trivially_relocatable<typename[:type_of(info):]>::value) {
                return true;
            }
        }
        return false;
    }();

template <typename Ty>
concept _eligible_for_relocation =
    !_has_any_virtual_base_class<Ty> &&
    !_has_not_trivially_relocatable_base_class<Ty> &&
    !_has_not_trivially_relocatable_nonstatic_data_member<Ty> &&
    std::destructible<Ty>;

template <typename Ty>
concept _has_trivially_relocatable_if_eligible = false;

template <typename Ty>
concept _union_without_user_defined_special_member_functions =
    std::is_union_v<Ty> && [] consteval {
        using namespace std::meta;
        auto ctx = access_context::current();
        for (auto info : members_of(^^Ty, ctx)) {
            if (is_special_member_function(info) && is_user_provided(info)) {
                return false;
            }
        }
        return true;
    }();

template <typename Ty>
concept _default_movable = [] consteval {
    using namespace std::meta;
    auto ctx = access_context::current();
    for (auto info : members_of(^^Ty, ctx)) {
        if (is_move_constructor(info) || is_move_assignment(info)) {
            if (is_user_provided(info)) {
                return false;
            }
        }
    }
    return true;
}();

template <typename Ty>
struct _is_trivially_relocatable {
    static constexpr bool value =
        _eligible_for_relocation<Ty> &&
        (_has_trivially_relocatable_if_eligible<Ty> ||
         _union_without_user_defined_special_member_functions<Ty> ||
         _default_movable<Ty>);
};
template <typename Ty>
requires std::is_fundamental_v<Ty>
struct _is_trivially_relocatable<Ty> : std::true_type {};

} // namespace _trivially_relocatable

template <typename Ty>
concept trivially_relocatable =
    _trivially_relocatable::_is_trivially_relocatable<Ty>::value;

} // namespace neutron

    #elif __has_builtin(__builtin_is_cpp_trivially_relocatable)

namespace neutron {

template <typename Ty>
concept trivially_relocatable = __builtin_is_cpp_trivially_relocatable(Ty);

}

    #elif !defined(__clang__) && __has_builtin(__is_trivially_relocatable)

namespace neutron {

/// @note: The implementation of `__is_trivially_relocatable` is since cxx03 and
/// it does not implement the semantics of any current or future trivial
/// relocation proposal and it can lead to incorrect optimizations on some
/// platforms (Windows) and supported compilers (AppleClang).
template <typename Ty>
concept trivially_relocatable = __is_trivially_relocatable(Ty);

} // namespace neutron

    #else

        #include <type_traits>

namespace neutron {

template <typename Ty>
concept trivially_relocatable = std::is_trivially_copyable_v<Ty>;

} // namespace neutron

    #endif

#endif
