#pragma once
#include <algorithm>
#include <cstring>
#include <format>
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

    constexpr auto operator==(const tstring& rhs) const noexcept {
        return view() == rhs.view();
    }

    template <std::size_t RLength>
    constexpr auto operator==(const tstring<RLength>& rhs) const noexcept
        -> bool {
        return false;
    }

    constexpr auto operator!=(const tstring& rhs) const noexcept {
        return view() != rhs.view();
    }

    template <std::size_t RLength>
    constexpr auto operator!=(const tstring<RLength>& obj) const -> bool {
        return false;
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

    NODISCARD constexpr size_t capacity() const noexcept { return Length; }

    NODISCARD constexpr std::string_view view() const noexcept { return value; }

    // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
    // NOLINTBEGIN(modernize-avoid-c-arrays)

    char value[Length]{};

    // NOLINTEND(modernize-avoid-c-arrays)
    // NOLINTEND(cppcoreguidelines-avoid-c-arrays)
};

} // namespace neutron

template <size_t Length>
struct std::formatter<neutron::tstring<Length>, char> {
    std::formatter<std::string_view, char> underlying;
    constexpr auto parse(auto& ctx) { return underlying.parse(ctx); }
    template <typename FormatContext>
    constexpr auto
        format(const neutron::tstring<Length>& ts, FormatContext& ctx) const {
        std::string_view sv{ ts.value, Length - 1 };
        return underlying.format(sv, ctx);
    }
};
