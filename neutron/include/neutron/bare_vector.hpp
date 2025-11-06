#pragma once
#include <compare>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include "neutron/neutron.hpp"
#include "neutron/ranges.hpp"

namespace neutron {

template <typename Ty>
class bare_vector;

template <typename Ty>
class _iter_wrapper;

template <typename Ty>
class _iter_wrapper<Ty*> {
    friend class bare_vector<std::remove_cvref_t<Ty>>;

    constexpr _iter_wrapper(Ty* iter) noexcept: iter_(iter) {}
public:
    using iterator_type     = Ty*;
    using value_type        = typename std::iterator_traits<iterator_type>::value_type;
    using difference_type   = typename std::iterator_traits<iterator_type>::difference_type;
    using pointer           = typename std::iterator_traits<iterator_type>::pointer;
    using reference         = typename std::iterator_traits<iterator_type>::reference;
    using iterator_category = typename std::iterator_traits<iterator_type>::iterator_category;
    using iterator_concept  = std::contiguous_iterator_tag;

    constexpr _iter_wrapper();

    template <typename T>
    constexpr _iter_wrapper(const _iter_wrapper<T>& that) noexcept;

    constexpr reference operator*() const noexcept;
    constexpr pointer operator->() const noexcept;
    constexpr _iter_wrapper& operator++();
    constexpr _iter_wrapper& operator++(int);
    constexpr _iter_wrapper& operator--();
    constexpr _iter_wrapper& operator--(int);
    constexpr _iter_wrapper operator+(difference_type n);
    constexpr _iter_wrapper& operator+=(difference_type n);
    constexpr _iter_wrapper operator-(difference_type n);
    constexpr _iter_wrapper& operator-=(difference_type n);

    constexpr iterator_type base() const noexcept { return iter_; }

private:
    iterator_type iter_;
};

template <typename It>
constexpr std::strong_ordering
    operator<=>(const _iter_wrapper<It>& lhs, const _iter_wrapper<It>& rhs) noexcept {
    if constexpr (std::three_way_comparable_with<It, It, std::strong_ordering>) {
        return lhs.base() <=> rhs.base();
    } else {
        if (lhs.base() < rhs.base()) {
            return std::strong_ordering::less;
        }
        if (lhs.base() == rhs.base()) {
            return std::strong_ordering::equal;
        }
        return std::strong_ordering::greater;
    }
}

template <typename It1, typename It2>
constexpr std::strong_ordering
    operator<=>(const _iter_wrapper<It1>& lhs, const _iter_wrapper<It2>& rhs) noexcept {
    if constexpr (std::three_way_comparable_with<It1, It2, std::strong_ordering>) {
        return lhs.base() <=> rhs.base();
    } else {
        if (lhs.base() < rhs.base()) {
            return std::strong_ordering::less;
        }
        if (lhs.base() == rhs.base()) {
            return std::strong_ordering::equal;
        }
        return std::strong_ordering::greater;
    }
}

template <typename It>
constexpr bool operator!=(const _iter_wrapper<It>& lhs, const _iter_wrapper<It>& rhs) noexcept {
    return lhs.base() != rhs.base();
}

template <typename It1, typename It2>
constexpr bool operator!=(const _iter_wrapper<It1>& lhs, const _iter_wrapper<It2>& rhs) noexcept {
    return lhs.base() != rhs.base();
}

template <typename Ty>
class bare_vector {
public:
    static_assert(sizeof(Ty));

    // clang-format off

    using value_type             = Ty;
    using pointer                = Ty*;
    using const_pointer          = const Ty*;
    using reference              = Ty&;
    using const_reference        = const Ty&;

    using iterator               = _iter_wrapper<pointer>;
    using const_iterator         = _iter_wrapper<const_pointer>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // clang-format on

    constexpr bare_vector() noexcept = default;

    template <std::input_iterator Iter, typename Sentinel, typename Alloc>
    constexpr bare_vector(Iter first, Sentinel last, Alloc& alloc);

    template <typename T, typename Alloc>
    constexpr bare_vector(std::initializer_list<T> il, Alloc& alloc);

#if HAS_CXX23
    template <std::ranges::input_range Rng, typename Alloc>
    constexpr bare_vector(std::from_range_t, Rng&& range, Alloc& alloc);
#endif

    constexpr bare_vector(const bare_vector& that) = delete;

    template <typename Alloc>
    constexpr bare_vector(const bare_vector& that, Alloc& alloc);

    constexpr bare_vector(bare_vector&& that) noexcept;

    constexpr bare_vector& operator=(const bare_vector& that) = delete;

    constexpr bare_vector& operator=(bare_vector&& that) noexcept;

    constexpr ~bare_vector() noexcept(std::is_nothrow_destructible_v<Ty>);

    template <typename Alloc>
    constexpr auto insert(const_iterator pos, const value_type& val);

    template <typename Alloc>
    constexpr auto insert(const_iterator pos, value_type&& val);

#if HAS_CXX23
    template <typename Alloc, concepts::compatible_range<value_type> Rng>
    constexpr auto insert_range(Alloc& alloc, const_iterator pos, Rng&& range);
#endif

    template <typename Alloc, typename... Args>
    constexpr auto emplace(Alloc& alloc, const_iterator pos, Args&&... args);

    template <typename Alloc, typename... Args>
    constexpr auto emplace_back(Alloc& alloc, Args&&... args);

    constexpr auto erase(const_iterator where) noexcept(std::is_nothrow_destructible_v<Ty>);

    constexpr auto clear();

private:
    Ty* data_{};
    size_t size_{};
    size_t capacity_{};
};

} // namespace neutron
