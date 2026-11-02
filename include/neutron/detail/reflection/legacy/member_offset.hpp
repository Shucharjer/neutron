// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include <array>
#include <cstdint>
#include "neutron/detail/concepts/value.hpp"
#include "neutron/detail/reflection/legacy/access.hpp"
#include "neutron/detail/reflection/legacy/concepts.hpp"
#include "neutron/detail/reflection/legacy/member_count.hpp"
#include "neutron/detail/reflection/legacy/tuple_view.hpp"
#include "neutron/detail/reflection/offset_helper.hpp"
#include "neutron/detail/string/tstring.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron::_refl_legacy {

using namespace ::neutron::_refl;

const auto offset_count = 2048;

template <aggregate Ty>
static inline const auto& offsets_of() noexcept {
    constexpr auto count = member_count_of<Ty>();
    auto& outline        = get_object_outline<Ty>();
    auto tuple           = object_to_tuple_view(outline);
    [[maybe_unused]] static std::array<size_t, count> array = {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            std::array<size_t, count> arr;
            ((arr[Is] =
                  (size_t)((const char*)std::addressof(std::get<Is>(tuple)) -
                           (char*)(std::addressof(outline)))),
             ...);
            return arr;
        }(std::make_index_sequence<count>())
    };
    return array;
}

template <aggregate Ty, typename... Args>
requires value<Ty>
consteval static auto
    make_offset_tuple(const std::tuple<Args...>& tuple) noexcept {
    return std::make_tuple(
        static_cast<std::remove_cv_t<std::remove_pointer_t<Args>> Ty::*>(
            nullptr)...);
}


template <aggregate Ty>
static inline auto offset_tuple() noexcept {
    static_assert(
        sizeof(Ty) < offset_count,
        "The size is larger than the max offset could get "
        "automatically! Please get the offsets manually.");
    constexpr auto view = struct_to_tuple_view<Ty>();
    auto tuple          = make_offset_tuple<Ty>(view);
    const auto& offsets = offsets_of<Ty>();

    // set offsets in tuple
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        ((std::get<Is>(tuple) = (std::tuple_element_t<Is, decltype(tuple)>)
              _offset_mapping(offsets[Is])),
         ...);
    }(std::make_index_sequence<member_count_of<Ty>()>());
    return tuple;
}

/*! @endcond */

template <default_reflectible_aggregate Ty>
inline const auto& offset_array_of() noexcept {
    return offsets_of<Ty>();
}

template <std::size_t Index, default_reflectible_aggregate Ty>
inline auto offset_value_of() noexcept {
    return offsets_of<Ty>()[Index];
}

template <default_reflectible_aggregate Ty>
inline const auto& offsets_of() noexcept {
    [[maybe_unused]] static const auto tuple = offset_tuple<Ty>();
    return tuple;
}

template <std::size_t Index, default_reflectible_aggregate Ty>
inline auto offset_of() noexcept {
    static_assert(Index < member_count_of<Ty>());
    const auto& offsets = offsets_of<Ty>();
    return std::get<Index>(offsets);
}

template <tstring Name, default_reflectible_aggregate Ty>
inline auto offset_of() noexcept {
    constexpr auto index = index_of<Name, Ty>();
    return offset_of<index, Ty>();
}

template <has_field_traits Ty>
[[nodiscard]] consteval auto offsets_of() noexcept {
    constexpr auto traits = Ty::field_traits();
    constexpr auto offsets =
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::make_tuple(std::get<Is>(traits).pointer()...);
        }(std::make_index_sequence<member_count_of<Ty>()>());
    return offsets;
}

template <std::size_t Index, has_field_traits Ty>
[[nodiscard]] consteval auto offset_of() noexcept {
    static_assert(Index < member_count_of<Ty>());
    return std::get<Index>(offsets_of<Ty>());
}

template <tstring Name, has_field_traits Ty>
requires(existance_of<Name, Ty>())
[[nodiscard]] constexpr auto offset_of() noexcept {
    return offset_of<index_of<Name, Ty>()>();
}

} // namespace neutron::_refl_legacy
