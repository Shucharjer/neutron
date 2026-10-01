// private, include <neutron/reflection.hpp>
#pragma once
#include "neutron/detail/macros.hpp"

#if ATOM_HAS_REFLECTION

    #include <array>
    #include <cstddef>
    #include <meta>
    #include <string_view>

namespace neutron {

namespace _refl_meta_name {

consteval std::string_view raw_name_of(std::meta::info info) noexcept {
    using namespace std::meta;
    while (is_type_alias(info)) {
        info = underlying_type(info);
    }
    return display_string_of(info);
}

constexpr bool is_tight_space(std::string_view raw, std::size_t pos) noexcept {
    if (pos == 0 || raw[pos] != ' ') {
        return false;
    }

    const auto next     = pos + 1 < raw.size() ? raw[pos + 1] : '\0';
    const auto previous = raw[pos - 1];
    return next == '*' || next == '&' || next == '(' || next == '[' ||
           previous == '*' || previous == '&' ||
           (previous == '>' && next == '>');
}

constexpr std::size_t normalized_size(std::string_view raw) noexcept {
    std::size_t size = 0;
    for (std::size_t index = 0; index < raw.size(); ++index) {
        if (!is_tight_space(raw, index)) {
            ++size;
        }
    }
    return size;
}

template <typename Ty>
consteval auto make_name_storage() noexcept {
    constexpr auto raw = raw_name_of(^^Ty);
    std::array<char, normalized_size(raw) + 1> storage{};
    std::size_t output = 0;
    for (std::size_t index = 0; index < raw.size(); ++index) {
        if (!is_tight_space(raw, index)) {
            storage[output++] = raw[index];
        }
    }
    return storage;
}

template <typename Ty>
inline constexpr auto name_storage = make_name_storage<Ty>();

} // namespace _refl_meta_name

template <typename Ty>
consteval std::string_view name_of() noexcept {
    return { _refl_meta_name::name_storage<Ty>.data(),
             _refl_meta_name::name_storage<Ty>.size() - 1 };
}

} // namespace neutron

#endif
