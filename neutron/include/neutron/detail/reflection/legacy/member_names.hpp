// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include <string_view>
#include "./tuple_view.hpp"

namespace neutron {

namespace _reflection {

template <auto Ptr> // name
consteval std::string_view member_name_of() noexcept {
#ifdef _MSC_VER
    constexpr std::string_view funcname = __FUNCSIG__;
#else
    constexpr std::string_view funcname = __PRETTY_FUNCTION__;
#endif

#ifdef __clang__
    auto split = funcname.substr(0, funcname.size() - 2);
    return split.substr(split.find_last_of(":.") + 1);
#elif defined(__GNUC__)
    auto split = funcname.substr(0, funcname.rfind(")}"));
    return split.substr(split.find_last_of(":") + 1);
#elif defined(_MSC_VER)
    auto split = funcname.substr(0, funcname.rfind("}>"));
    return split.substr(split.rfind("->") + 2);
#else
    static_assert(false, "Unsupportted compiler");
#endif
}

template <typename Ty>
struct wrapper {
    Ty val;
};

template <typename Ty>
wrapper(Ty) -> wrapper<Ty>;

template <typename Ty>
constexpr static auto wrap(const Ty& arg) noexcept {
    return wrapper{ arg };
}

/**
 * @brief Get members' name of a type.
 *
 */
template <reflectible Ty>
constexpr std::array<std::string_view, member_count_of<Ty>()>
    member_names_of() noexcept {
    constexpr size_t count = member_count_of<Ty>();
    std::array<std::string_view, count> array;
    if constexpr (default_reflectible_aggregate<Ty>) {
        constexpr auto tuple = struct_to_tuple_view<Ty>();
        [&]<size_t... Is>(std::index_sequence<Is...>) {
            ((array[Is] = member_name_of<wrap(std::get<Is>(tuple))>()), ...);
        }(std::make_index_sequence<count>());
    } else { // has_field_traits
        constexpr auto tuple = Ty::field_traits();
        [&]<size_t... Is>(std::index_sequence<Is...>) {
            ((array[Is] = std::get<Is>(tuple).name()), ...);
        }(std::make_index_sequence<count>());
    }
    return array;
}

} // namespace _reflection

using _reflection::member_names_of;

} // namespace neutron
