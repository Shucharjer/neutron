// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once

#ifndef __has_feature
    #define __has_feature(x) 0
#endif

#if __has_feature(__cpp_trivial_relocatability) &&                             \
    __cpp_trivial_relocatability >= 202502L && false

namespace neutron {

template <typename Ty>
using trivially_relocatable =
    std::is_trivially_relocatable_v<Ty>; // the prossible interface

}

#else

    #include "neutron/detail/utility/macros.hpp"

    #if ATOM_HAS_REFLECTION

        #include <algorithm>
        #include <meta>
        #include <ranges>
        #include <vector>

namespace neutron {

namespace _trivially_relocatable {

consteval bool _is_trivially_relocatable(std::meta::info type);

/// @note Private bases and members must be inspected as well, so all queries
/// use an unchecked access context.
consteval std::meta::access_context _context() noexcept {
    return std::meta::access_context::unchecked();
}

consteval bool _has_no_virtual_base_class(std::meta::info type) {
    return std::ranges::none_of(
        std::meta::bases_of(type, _context()), [](std::meta::info base) {
            return std::meta::is_virtual(base);
        });
}

consteval bool _is_trivially_relocatable_subobject(std::meta::info subobject) {
    const auto type = std::meta::type_of(subobject);
    return std::meta::is_reference_type(type) ||
           _is_trivially_relocatable(type);
}

consteval bool _has_trivially_relocatable_subobjects(std::meta::info type) {
    return std::ranges::all_of(
               std::meta::bases_of(type, _context()),
               _is_trivially_relocatable_subobject) &&
           std::ranges::all_of(
               std::meta::nonstatic_data_members_of(type, _context()),
               _is_trivially_relocatable_subobject);
}

consteval bool _is_eligible_for_relocation(std::meta::info type) {
    return _has_no_virtual_base_class(type) &&
           _has_trivially_relocatable_subobjects(type) &&
           std::meta::is_destructible_type(type);
}

consteval bool _is_trivially_relocatable_if_eligible(std::meta::info) {
    return false; // `trivially_relocatable_if_eligible` is not implemented
}

consteval bool _is_union_without_user_declared_special_member_functions(
    std::meta::info type) {
    return std::meta::is_union_type(type) &&
           std::ranges::none_of(
               std::meta::members_of(type, _context()),
               [](std::meta::info member) {
                   return std::meta::is_special_member_function(member) &&
                          std::meta::is_user_declared(member);
               });
}

consteval bool _is_neither_user_provided_nor_deleted(std::meta::info member) {
    return !std::meta::is_user_provided(member) &&
           !std::meta::is_deleted(member);
}

/// @brief Checks the special member selected for an xvalue of the class.
/// @note A defaulted move operation defined as deleted is ignored by overload
/// resolution, so the copy operation is selected instead.
consteval bool _selects_default_special_member(
    const std::vector<std::meta::info>& members, auto is_move, auto is_copy) {
    auto moves =
        members | std::views::filter([is_move](std::meta::info member) {
            return is_move(member) && !(std::meta::is_defaulted(member) &&
                                        std::meta::is_deleted(member));
        });
    if (!std::ranges::empty(moves)) {
        return std::ranges::all_of(
            moves, _is_neither_user_provided_nor_deleted);
    }
    return std::ranges::all_of(
        members | std::views::filter(is_copy),
        _is_neither_user_provided_nor_deleted);
}

consteval bool _is_default_movable(std::meta::info type) {
    const auto members = std::meta::members_of(type, _context());
    return _selects_default_special_member(
               members,
               [](std::meta::info member) {
                   return std::meta::is_move_constructor(member);
               },
               [](std::meta::info member) {
                   return std::meta::is_copy_constructor(member);
               }) &&
           _selects_default_special_member(
               members,
               [](std::meta::info member) {
                   return std::meta::is_move_assignment(member);
               },
               [](std::meta::info member) {
                   return std::meta::is_copy_assignment(member);
               }) &&
           std::ranges::all_of(
               members | std::views::filter([](std::meta::info member) {
                   return std::meta::is_destructor(member);
               }),
               _is_neither_user_provided_nor_deleted);
}

consteval bool _is_trivially_relocatable(std::meta::info type) {
    type = std::meta::remove_cv(std::meta::remove_all_extents(type));
    if (std::meta::is_scalar_type(type)) {
        return true;
    }

    // void, references and functions are not relocatable
    if (!std::meta::is_class_type(type) && !std::meta::is_union_type(type)) {
        return false;
    }

    return _is_eligible_for_relocation(type) &&
           (_is_trivially_relocatable_if_eligible(type) ||
            _is_union_without_user_declared_special_member_functions(type) ||
            _is_default_movable(type));
}

} // namespace _trivially_relocatable

template <typename Ty>
concept trivially_relocatable =
    _trivially_relocatable::_is_trivially_relocatable(^^Ty);

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
