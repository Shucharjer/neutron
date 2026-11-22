// IWYU pragma: private, include <neutron/string.hpp>
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <string_view>
#include <type_traits>
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

/**
 * @brief A fixed-capacity string stored inline that truncates instead of
 * growing.
 *
 * Assigning a longer string keeps its first `Capacity` characters and records
 * the truncation; it never allocates and never throws. The object is
 * trivially copyable, so it can be copied as raw bytes, e.g. through a
 * lock-free queue or shared memory. The contents are not null-terminated.
 *
 * @tparam CharT The character type.
 * @tparam Capacity The maximum number of characters kept.
 */
template <typename CharT, std::size_t Capacity>
requires(Capacity != 0 && Capacity <= std::numeric_limits<std::uint16_t>::max())
class basic_inplace_string {
public:
    using value_type = CharT;
    using size_type  = std::size_t;

    constexpr basic_inplace_string() noexcept = default;

    constexpr basic_inplace_string(std::basic_string_view<CharT> str) noexcept {
        assign(str);
    }

    constexpr basic_inplace_string(const CharT* str) noexcept
        : basic_inplace_string(std::basic_string_view<CharT>{ str }) {}

    /// @brief Replaces the contents, keeping at most `Capacity` characters.
    constexpr void assign(std::basic_string_view<CharT> str) noexcept {
        const auto count = (std::min)(str.size(), Capacity);
        std::copy_n(str.data(), count, data_);
        size_      = static_cast<std::uint16_t>(count);
        truncated_ = str.size() > Capacity;
    }

    ATOM_NODISCARD constexpr std::basic_string_view<CharT>
        view() const noexcept {
        return { data_, size_ };
    }

    ATOM_NODISCARD constexpr const CharT* data() const noexcept {
        return data_;
    }

    ATOM_NODISCARD constexpr size_type size() const noexcept { return size_; }

    ATOM_NODISCARD constexpr bool empty() const noexcept { return size_ == 0; }

    ATOM_NODISCARD static constexpr size_type capacity() noexcept {
        return Capacity;
    }

    /// @brief Whether the last assignment dropped characters.
    ATOM_NODISCARD constexpr bool truncated() const noexcept {
        return truncated_;
    }

    ATOM_NODISCARD constexpr bool
        operator==(const basic_inplace_string& that) const noexcept {
        return view() == that.view();
    }

private:
    CharT data_[Capacity]{}; // NOLINT(modernize-avoid-c-arrays)
    std::uint16_t size_ = 0;
    bool truncated_     = false;
};

template <std::size_t Capacity>
using inplace_string = basic_inplace_string<char, Capacity>;

} // namespace neutron

template <typename CharT, std::size_t Capacity>
struct std::formatter<neutron::basic_inplace_string<CharT, Capacity>, CharT> {
    std::formatter<std::basic_string_view<CharT>, CharT> underlying;
    constexpr auto parse(auto& ctx) { return underlying.parse(ctx); }
    template <typename FormatContext>
    constexpr auto format(
        const neutron::basic_inplace_string<CharT, Capacity>& str,
        FormatContext& ctx) const {
        return underlying.format(str.view(), ctx);
    }
};
