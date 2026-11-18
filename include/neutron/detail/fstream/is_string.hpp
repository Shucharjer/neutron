// IWYU pragma: private, include <neutron/fstream.hpp>
#pragma once
#include <concepts>
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

/// @brief A type providing a null-terminated narrow string via `c_str()`.
template <typename String>
concept _is_string = requires(const String& string) {
    { string.c_str() } -> std::convertible_to<const char*>;
};

/// @brief A narrow, null-terminated file path.
template <typename Path>
concept _file_path =
    std::convertible_to<const Path&, const char*> || _is_string<Path>;

template <_file_path Path>
ATOM_NODISCARD constexpr const char* _path_c_str(const Path& path) {
    if constexpr (std::convertible_to<const Path&, const char*>) {
        return path;
    } else {
        return path.c_str();
    }
}

} // namespace neutron
