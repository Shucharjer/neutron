// IWYU pragma: private, include <neutron/iterator.hpp>
#pragma once
#include <iterator>
#include <type_traits>

namespace neutron {

template <typename Ty>
class _iter_wrapper;

template <typename Ty>
class _iter_wrapper<Ty*> {
public:
    using iterator_type = Ty*;
    using value_type = typename std::iterator_traits<iterator_type>::value_type;
    using difference_type =
        typename std::iterator_traits<iterator_type>::difference_type;
    using pointer   = typename std::iterator_traits<iterator_type>::pointer;
    using reference = typename std::iterator_traits<iterator_type>::reference;
    using iterator_category =
        typename std::iterator_traits<iterator_type>::iterator_category;
    using iterator_concept = std::contiguous_iterator_tag;

    constexpr _iter_wrapper(Ty* iter) noexcept : iter_(iter) {}

    constexpr _iter_wrapper() noexcept = default;

    template <typename T>
    constexpr _iter_wrapper(const _iter_wrapper<T>& that) noexcept;

    constexpr reference operator*() const noexcept { return *iter_; }
    constexpr pointer operator->() const noexcept { return iter_; }
    constexpr _iter_wrapper& operator++() noexcept {
        ++iter_;
        return *this;
    }
    constexpr _iter_wrapper operator++(int) noexcept {
        auto iter = iter_;
        return _iter_wrapper{ ++iter };
    }
    constexpr _iter_wrapper& operator--() noexcept {
        --iter_;
        return *this;
    }
    constexpr _iter_wrapper operator--(int) noexcept {
        auto iter = iter_;
        return _iter_wrapper{ --iter };
    }
    constexpr _iter_wrapper operator+(difference_type n) noexcept {
        return _iter_wrapper{ iter_ + n };
    }
    constexpr _iter_wrapper& operator+=(difference_type n) noexcept {
        iter_ += n;
        return *this;
    }
    constexpr _iter_wrapper operator-(difference_type n) noexcept {
        return _iter_wrapper{ iter_ - n };
    }
    constexpr _iter_wrapper& operator-=(difference_type n) noexcept {
        iter_ -= n;
        return *this;
    }

    constexpr iterator_type base() const noexcept { return iter_; }

private:
    iterator_type iter_;
};

template <typename It>
constexpr std::strong_ordering operator<=>(
    const _iter_wrapper<It>& lhs, const _iter_wrapper<It>& rhs) noexcept {
    if constexpr (std::three_way_comparable_with<
                      It, It, std::strong_ordering>) {
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
constexpr std::strong_ordering operator<=>(
    const _iter_wrapper<It1>& lhs, const _iter_wrapper<It2>& rhs) noexcept {
    if constexpr (std::three_way_comparable_with<
                      It1, It2, std::strong_ordering>) {
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
constexpr bool operator!=(
    const _iter_wrapper<It>& lhs, const _iter_wrapper<It>& rhs) noexcept {
    return lhs.base() != rhs.base();
}

template <typename It1, typename It2>
constexpr bool operator!=(
    const _iter_wrapper<It1>& lhs, const _iter_wrapper<It2>& rhs) noexcept {
    return lhs.base() != rhs.base();
}

} // namespace neutron
