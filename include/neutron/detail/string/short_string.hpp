// IWYU pragma: private, include <neutron/string.hpp>
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <format>
#include <initializer_list>
#include <ios>
#include <istream>
#include <iterator>
#include <locale>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include "neutron/detail/iterator/iter_wrapper.hpp"
#include "neutron/detail/memory/inplace.hpp"
#include "neutron/detail/memory/raw_memory.hpp"
#include "neutron/detail/string/string_view_like.hpp"
#include "neutron/detail/utility/macros.hpp"

#include <version>
#if __cpp_lib_containers_ranges >= 202202L
    #include <ranges>
    #include "neutron/detail/ranges/concepts.hpp"
#endif

namespace std {}

namespace neutron {

template <
    std::size_t Capacity, typename CharT,
    typename Traits = std::char_traits<CharT>,
    typename Alloc  = std::allocator<CharT>>
class basic_short_string;

template <std::size_t Capacity>
using short_string = basic_short_string<Capacity, char>;

template <std::size_t Capacity>
using short_wstring = basic_short_string<Capacity, wchar_t>;

template <std::size_t Capacity>
using short_u8string = basic_short_string<Capacity, char8_t>;

template <std::size_t Capacity>
using short_u16string = basic_short_string<Capacity, char16_t>;

template <std::size_t Capacity>
using short_u32string = basic_short_string<Capacity, char32_t>;

template <std::size_t Capacity, typename CharT, typename Traits, typename Alloc>
class basic_short_string : Alloc, inplace_storage<CharT, Capacity, true> {
    using _storage_t = inplace_storage<CharT, Capacity, true>;

public:
    using traits_type            = Traits;
    using value_type             = traits_type::char_type;
    using allocator_type         = Alloc;
    using alloc_traits           = std::allocator_traits<allocator_type>;
    using size_type              = allocator_type::size_type;
    using difference_type        = allocator_type::difference_type;
    using reference              = CharT&;
    using const_reference        = const CharT&;
    using pointer                = alloc_traits::pointer;
    using const_pointer          = alloc_traits::const_pointer;
    using iterator               = _iter_wrapper<pointer>;
    using const_iterator         = _iter_wrapper<const_pointer>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<iterator>;

    static constexpr size_type npos = static_cast<size_type>(-1);

    constexpr basic_short_string() noexcept(noexcept(allocator_type()))
        : basic_short_string(allocator_type()) {}

    explicit constexpr basic_short_string(const allocator_type& alloc)
        : Alloc(alloc) {}

    constexpr basic_short_string(
        size_type count, CharT ch, const Alloc& alloc = {});

    template <typename InputIter>
    constexpr basic_short_string(
        InputIter first, InputIter last, const Alloc& = {}) {
        if constexpr (std::forward_iterator<InputIter>) {
            const auto n = static_cast<size_type>(std::distance(first, last));
            if (n > Capacity - 1) {
                data_ = alloc_traits::allocate(*this, n);
            }
            size_ = uninitialized_copy_n(first, last, data_) - data_;
            _set_tail_zero();
        } else {
            auto guard = make_exception_guard([this]() noexcept {
                if (!_is_short()) {
                    alloc_traits(*this, data_);
                    data_     = _storage_t::data();
                    capacity_ = Capacity;
                }
            });
            for (; first != last; ++first, ++size_) {
                if (_need_grow()) {
                    _grow_to(_next_capacity());
                }

                data_[size_] = *first;
            }
        }
    }

#if __cpp_lib_containers_ranges >= 202202L
    template <compatible_range<CharT> Rng>
    constexpr basic_short_string(
        std::from_range_t, Rng&& range, const allocator_type& alloc = {});
#endif

    constexpr basic_short_string(
        const CharT* str, size_type count, const allocator_type& alloc = {}) {
        if (count > Capacity - 1) {
            data_     = alloc_traits::allocate(*this, count + 1);
            capacity_ = count + 1;
        }
        uninitialized_copy_n(str, count, data_);
        size_ = count;
        _set_tail_zero();
    }

    template <std::size_t Cap>
    constexpr basic_short_string(
        const char (&literal)[Cap], const allocator_type& alloc = {}) {
        if constexpr (Cap > Capacity) {
            data_     = alloc_traits::allocate(*this, Cap);
            capacity_ = Cap;
        }
        uninitialized_copy_n(literal, Cap, data_);
        size_ = Cap - 1;
        _set_tail_zero();
    }

    basic_short_string(std::nullptr_t) = delete;

    template <string_view_like StringViewLike>
    explicit basic_short_string(
        const StringViewLike& sv, const allocator_type& alloc = {})
        : basic_short_string(sv.begin(), sv.end(), alloc) {}

    template <string_view_like StringViewLike>
    constexpr basic_short_string(
        const StringViewLike& sv, size_type pos, size_type count,
        const allocator_type& alloc = {})
        : basic_short_string(
              sv.begin() + pos, sv.begin() + pos + count, alloc) {}

    constexpr basic_short_string(const basic_short_string& other) {
        if (!other._is_short()) {
            data_     = alloc_traits::allocate(*this, other.capacity_);
            capacity_ = other.capacity_;
        }
        uninitialized_copy_n(other.data_, other.size_ + 1, data_);
        size_ = other.size_;
    }

    constexpr basic_short_string(basic_short_string&& other) noexcept;

    constexpr basic_short_string(
        const basic_short_string& other, const allocator_type& alloc);

    constexpr basic_short_string(
        basic_short_string&& other,
        const allocator_type& alloc = {}) noexcept(false);

    constexpr basic_short_string(
        const basic_short_string& other, size_type pos, size_type count,
        const allocator_type& alloc = {});

    constexpr basic_short_string(
        basic_short_string&& other, size_type pos, size_type count,
        const allocator_type& alloc = {});

    constexpr basic_short_string(
        std::initializer_list<value_type> ilist,
        const allocator_type& alloc = {});

    constexpr basic_short_string& operator=(const basic_short_string&) = delete;

    constexpr basic_short_string&
        operator=(basic_short_string&&) noexcept = delete;

    constexpr ~basic_short_string() noexcept {
        if (data_ != _storage_t::data()) {
            Alloc::deallocate(data_, capacity_);
        }
    }

    constexpr auto get_allocator() const noexcept -> const allocator_type& {
        return static_cast<const allocator_type&>(*this);
    }

    constexpr auto at(size_type pos) -> CharT& {
        if (pos > size_) [[unlikely]] {
            throw std::out_of_range("");
        }

        return data_[pos];
    }

    constexpr auto at(size_type pos) const -> const CharT& {
        if (pos > size_) [[unlikely]] {
            throw std::out_of_range("");
        }

        return data_[pos];
    }

    // undefined behavior until c++26
    constexpr auto operator[](size_type pos) noexcept -> reference {
        return data_[pos];
    }

    // if pos >= size, it is a undefined behavior until c++26
    constexpr auto operator[](size_type pos) const noexcept -> const_reference {
        return data_[pos];
    }

    constexpr auto front() noexcept -> reference { return data_[0]; }

    constexpr auto front() const noexcept -> const_reference {
        return data_[0];
    }

    constexpr auto back() noexcept -> reference { return data_[size_ - 1]; }

    constexpr auto back() const noexcept -> const_reference {
        return data_[size_ - 1];
    }

    constexpr auto data() noexcept -> pointer { return data_; }

    constexpr auto data() const noexcept -> const_pointer { return data_; }

    constexpr auto c_str() const noexcept -> const_pointer { return data_; }

    constexpr operator std::basic_string_view<CharT, Traits>() const noexcept {
        return { data_, size_ };
    }

    constexpr iterator begin() noexcept { return data_; }

    constexpr const_iterator begin() const noexcept { return data_; }

    constexpr const_iterator cbegin() const noexcept { return data_; }

    constexpr iterator end() noexcept { return data_ + size_; }

    constexpr const_iterator end() const noexcept { return data_ + size_; }

    constexpr const_iterator cend() const noexcept { return data_ + size_; }

    ATOM_NODISCARD constexpr bool empty() const noexcept { return size_ == 0; }

    ATOM_NODISCARD constexpr size_type size() const noexcept { return size_; }

    ATOM_NODISCARD constexpr size_type length() const noexcept { return size_; }

    ATOM_NODISCARD constexpr size_type max_size() const noexcept {
        return static_cast<size_type>(-1);
    }

    ATOM_NODISCARD constexpr size_type capacity() const noexcept {
        return capacity_;
    }

    constexpr void clear() noexcept {
        size_ = 0;
        if constexpr (Capacity == 0) {
            if (data_ == nullptr) {
                return;
            }
        }
        _set_tail_zero();
    }

    constexpr void push_back(CharT ch) {
        if (_need_grow()) [[unlikely]] {
            _grow_to(_next_capacity());
        }
        std::construct_at(std::to_address(data_ + size_), ch);
        ++size_;
        _set_tail_zero();
    }

    constexpr basic_short_string& operator+=(const basic_short_string& str);

    constexpr basic_short_string& operator+=(CharT ch);

    constexpr basic_short_string& operator+=(const CharT* str);

    constexpr basic_short_string&
        operator+=(std::initializer_list<CharT> ilist);

    template <string_view_like StringViewLike>
    constexpr basic_short_string& operator+=(const StringViewLike& sv);

    ATOM_NODISCARD constexpr auto
        find(const basic_short_string& str, size_type pos = 0) const noexcept {
        return std::string_view{ data_, size_ }.find(str, pos);
    }

    constexpr void swap(basic_short_string& other) noexcept;

private:
    ATOM_NODISCARD ATOM_ALWAYS_INLINE constexpr bool
        _is_short() const noexcept {
        return data_ == _storage_t::data();
    }

    /// @brief Whether one more character leaves no room for the terminator.
    ATOM_NODISCARD ATOM_ALWAYS_INLINE constexpr bool
        _need_grow() const noexcept {
        return size_ + 1 >= capacity_;
    }

    ATOM_NODISCARD ATOM_ALWAYS_INLINE constexpr size_type
        _next_capacity() const noexcept {
        if (capacity_ < 2) [[unlikely]] {
            return 2;
        }
        return capacity_ << 1;
    }

    ATOM_ALWAYS_INLINE constexpr void _set_tail_zero() noexcept {
        std::construct_at(std::to_address(data_ + size_), CharT());
    }

    /// @brief Moves the contents into a new buffer of `n` characters.
    /// @note Strong exception guarantee: nothing changes if allocation throws.
    constexpr void _grow_to(size_type n) {
        pointer p = alloc_traits::allocate(*this, n);
        uninitialized_copy_n(data_, size_, p);
        std::construct_at(std::to_address(p + size_), CharT());
        if (!_is_short()) {
            alloc_traits::deallocate(*this, data_, capacity_);
        }
        data_     = p;
        capacity_ = n;
    }

    pointer data_{ _storage_t::data() };
    size_type size_{};
    size_type capacity_{ Capacity };
};

// NOLINTBEGIN(modernize-avoid-c-arrays)

template <std::size_t Size>
basic_short_string(const char (&)[Size]) -> basic_short_string<Size, char>;

template <std::size_t Size>
basic_short_string(const wchar_t (&)[Size])
    -> basic_short_string<Size, wchar_t>;

template <std::size_t Size>
basic_short_string(const char8_t (&)[Size])
    -> basic_short_string<Size, char8_t>;

template <std::size_t Size>
basic_short_string(const char16_t (&)[Size])
    -> basic_short_string<Size, char16_t>;

template <std::size_t Size>
basic_short_string(const char32_t (&)[Size])
    -> basic_short_string<Size, char32_t>;

// NOLINTEND(modernize-avoid-c-arrays)

template <std::size_t Capacity, typename CharT, typename Traits, typename Alloc>
::std::basic_ostream<CharT, Traits>& operator<<(
    ::std::basic_ostream<CharT, Traits>& os,
    const basic_short_string<Capacity, CharT, Traits, Alloc>& ss) {
    os << static_cast<std::basic_string_view<CharT, Traits>>(ss);
    return os;
}

/**
 * @brief Extracts a whitespace-delimited word, like `std::basic_string`.
 *
 * Leading whitespace is skipped, then characters are read until whitespace
 * (left in the stream), end of file or `is.width()` characters when it is
 * positive. `is.width(0)` is called afterwards. `failbit` is set if nothing
 * was extracted. If an exception is thrown during extraction, `badbit` is set
 * and the exception is rethrown when `badbit` is in `is.exceptions()`.
 */
template <std::size_t Capacity, typename CharT, typename Traits, typename Alloc>
::std::basic_istream<CharT, Traits>& operator>>(
    ::std::basic_istream<CharT, Traits>& is,
    basic_short_string<Capacity, CharT, Traits, Alloc>& ss) {
    using istream_t = ::std::basic_istream<CharT, Traits>;
    using size_type =
        typename basic_short_string<Capacity, CharT, Traits, Alloc>::size_type;

    ::std::ios_base::iostate state = ::std::ios_base::goodbit;
    size_type extracted            = 0;
    const typename istream_t::sentry sentry(is);
    if (sentry) {
        ATOM_TRY {
            ss.clear();
            const ::std::streamsize width = is.width();
            const size_type limit =
                width > 0 ? static_cast<size_type>(width) : ss.max_size();
            const auto& ctype =
                ::std::use_facet<::std::ctype<CharT>>(is.getloc());
            auto* const buf = is.rdbuf();
            for (; extracted < limit; ++extracted) {
                const auto ich = buf->sgetc();
                if (Traits::eq_int_type(ich, Traits::eof())) {
                    state |= ::std::ios_base::eofbit;
                    break;
                }
                const CharT ch = Traits::to_char_type(ich);
                if (ctype.is(::std::ctype_base::space, ch)) {
                    break;
                }
                ss.push_back(ch);
                buf->sbumpc();
            }
            is.width(0);
        }
        ATOM_CATCH(...) {
            // setstate() throws ios_base::failure after setting the state, but
            // the original exception is the one to propagate.
            ATOM_TRY { is.setstate(::std::ios_base::badbit); }
            ATOM_CATCH(const ::std::ios_base::failure&) {}
            if (is.exceptions() & ::std::ios_base::badbit) {
                ATOM_RETHROW;
            }
            return is;
        }
    }
    if (extracted == 0) {
        state |= ::std::ios_base::failbit;
    }
    is.setstate(state);
    return is;
}

} // namespace neutron

namespace std {

template <std::size_t Capacity, typename CharT, typename Traits, typename Alloc>
struct hash<::neutron::basic_short_string<Capacity, CharT, Traits, Alloc>> {
    std::size_t operator()(
        const ::neutron::basic_short_string<Capacity, CharT, Traits, Alloc>& ss)
        const noexcept {
        return std::hash<std::string_view>()(ss);
    }
};

template <size_t Capacity, typename CharT, typename Traits, typename Alloc>
struct formatter<
    ::neutron::basic_short_string<Capacity, CharT, Traits, Alloc>, CharT> {
    using short_string_t =
        neutron::basic_short_string<Capacity, CharT, Traits, Alloc>;

    constexpr auto parse(basic_format_parse_context<CharT>& pc) {
        return formatter<string_view, CharT>{}.parse(pc);
    }

    auto format(const short_string_t& ss, format_context& ctx) const {
        return formatter<string_view, CharT>{}.format(ss, ctx);
    }
};

} // namespace std
