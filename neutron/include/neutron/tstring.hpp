#pragma once
#include <algorithm>
#include <cstring>
#include <string_view>
#include "../src/neutron/internal/macros.hpp"

namespace neutron {

template <std::size_t Length>
struct tstring {
    // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
    // NOLINTBEGIN(modernize-avoid-c-arrays)

    constexpr tstring(const char (&arr)[Length]) noexcept {
        // memcpy is not a compile-time function
        // std::memcpy(val, arr, N);
        if CONST_EVALUATED {
            std::copy(arr, arr + Length, value);
        } else {
            std::memcpy(value, arr, Length);
        }
    }

    // NOLINTEND(modernize-avoid-c-arrays)
    // NOLINTEND(cppcoreguidelines-avoid-c-arrays)

    template <std::size_t RLength>
    constexpr auto operator<=>(const tstring<RLength>& rhs) const noexcept {
        return std::strcmp(value, rhs.value);
    }

    template <std::size_t RLength>
    constexpr auto operator==(const tstring<RLength>& rhs) const noexcept
        -> bool {
        return !(*this <=> rhs);
    }

    template <std::size_t RLength>
    constexpr auto operator!=(const tstring<RLength>& obj) const -> bool {
        return (*this <=> obj);
    }

    template <typename OStream>
    constexpr OStream& operator<<(OStream& stream) {
        stream << value;
        return stream;
    }

    NODISCARD constexpr bool empty() const noexcept { return Length == 1; }

    NODISCARD constexpr const char* data() const noexcept { return value; }

    NODISCARD constexpr const char* c_str() const noexcept { return value; }

    NODISCARD constexpr size_t size() const noexcept { return Length - 1; }

    NODISCARD constexpr size_t length() const noexcept { return Length - 1; }

    NODISCARD constexpr std::string_view view() const noexcept { return value; }

    // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
    // NOLINTBEGIN(modernize-avoid-c-arrays)

    char value[Length]{};

    // NOLINTEND(modernize-avoid-c-arrays)
    // NOLINTEND(cppcoreguidelines-avoid-c-arrays)
};

template <size_t Length>
struct tstring_view {
    std::string_view view;
    constexpr tstring_view(const tstring<Length>& tstring) noexcept
        : view(tstring.value) {}
    constexpr operator std::string_view() const noexcept { return view; }
    constexpr bool operator==(std::string_view rvw) const noexcept {
        return view == rvw;
    }
};

} // namespace neutron
