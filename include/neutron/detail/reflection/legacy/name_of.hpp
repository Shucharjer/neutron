// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include <string_view>
#include "neutron/detail/macros.hpp"

namespace neutron::_refl_legacy {

template <typename Ty>
ATOM_NODISCARD consteval std::string_view raw_function_name() noexcept {
#ifdef _MSC_VER
    return __FUNCSIG__;
#else
    return __PRETTY_FUNCTION__;
#endif
}

ATOM_NODISCARD consteval std::string_view
    extract_type_name(std::string_view funcname) noexcept {
    constexpr std::string_view marker = "Ty = ";

    const auto begin = funcname.find(marker);
    if (begin == std::string_view::npos) {
        return {};
    }

    const auto start = begin + marker.size();

#ifdef __clang__
    const auto end = funcname.rfind(']');
#elif defined(__GNUC__)
    const auto end = [&] {
        const auto alias = funcname.find(';', start);
        if (alias != std::string_view::npos) {
            return alias;
        }
        return funcname.rfind(']');
    }();
#else
    const auto end = std::string_view::npos;
#endif

    if (end == std::string_view::npos || end < start) {
        return {};
    }

    return funcname.substr(start, end - start);
}

template <typename Ty>
ATOM_NODISCARD consteval std::string_view compiler_name_of() noexcept {
#if defined(__clang__) || defined(__GNUC__)
    return extract_type_name(raw_function_name<Ty>());
#elif defined(_MSC_VER)
    constexpr auto funcname = raw_function_name<Ty>();
    auto split              = funcname.substr(110);
    split                   = split.substr(split.find_first_of(' ') + 1);
    return split.substr(0, split.size() - 7);
#else
    static_assert(false, "Unsupportted compiler");
#endif
}

/**
 * @brief Customization point for legacy type names.
 *
 */
template <typename Ty>
struct name_of_type {
    static constexpr auto value = _refl_legacy::compiler_name_of<Ty>();
};

/**
 * @brief Get the name of a type.
 *
 */
template <typename Ty>
ATOM_NODISCARD consteval std::string_view name_of() noexcept {
    return std::string_view{ name_of_type<Ty>::value };
}

} // namespace neutron::_refl_legacy
