// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include <string_view>

namespace neutron {

/**
 * @brief Get the name of a type.
 *
 */
template <typename Ty>
[[nodiscard]] consteval std::string_view name_of() noexcept {
#ifdef _MSC_VER
    constexpr std::string_view funcname = __FUNCSIG__;
#else
    constexpr std::string_view funcname = __PRETTY_FUNCTION__;
#endif

#ifdef __clang__
    auto split = funcname.substr(0, funcname.size() - 1);
    split      = split.substr(split.find_last_of('=') + 2);
    auto pos   = split.find_last_of(':');
    if (pos != std::string_view::npos) {
        return split.substr(pos + 1);
    }
    return split;
#elif defined(__GNUC__)
    auto split = funcname.substr(77);
    return split.substr(0, split.size() - 50);
#elif defined(_MSC_VER)
    auto split = funcname.substr(110);
    split      = split.substr(split.find_first_of(' ') + 1);
    return split.substr(0, split.size() - 7);
#else
    static_assert(false, "Unsupportted compiler");
#endif
}

} // namespace neutron
