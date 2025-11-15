#include <cstddef>
#include <ranges>
#include "neutron/internal/get.hpp"
#include "neutron/internal/range_adaptor_closure.hpp"

namespace neutron::ranges::views {

template <std::ranges::range Rng, size_t Index, bool IsConst>
struct element_iterator {
    using iterator_type = std::ranges::iterator_t<Rng>;
    using value_type =
        std::tuple_element_t<Index, std::ranges::range_value_t<Rng>>;
    using difference_type = ptrdiff_t;

    constexpr element_iterator() = default;

    constexpr element_iterator(std::ranges::iterator_t<Rng> iter) noexcept(
        std::is_nothrow_move_constructible_v<std::ranges::iterator_t<Rng>>)
        : iter_(std::move(iter)) {}

    [[nodiscard]] constexpr decltype(auto) operator*() noexcept
    requires(!IsConst)
    {
        if constexpr (gettible<
                          Index, std::remove_const_t<
                                     std::ranges::range_value_t<Rng>>>) {
            return _get<Index>(*iter_);
        } else {
            static_assert(false, "No suitable method to get the value.");
        }
    }

    [[nodiscard]] constexpr decltype(auto) operator*() const noexcept {
        if constexpr (gettible<
                          Index, std::remove_const_t<
                                     std::ranges::range_value_t<Rng>>>) {
            return _get<Index>(*iter_);
        } else {
            static_assert(false, "No suitable method to get the value.");
        }
    }

    [[nodiscard]] constexpr decltype(auto) operator->() noexcept
    requires(!IsConst)
    {
        return iter_;
    }

    [[nodiscard]] constexpr decltype(auto) operator->() const noexcept {
        return iter_;
    }

    constexpr element_iterator& operator++() noexcept(noexcept(++iter_)) {
        ++iter_;
        return *this;
    }

    constexpr element_iterator operator++(int) noexcept(
        noexcept(++iter_) &&
        std::is_nothrow_copy_constructible_v<std::ranges::iterator_t<Rng>>) {
        auto temp = *this;
        ++iter_;
        return temp;
    }

    constexpr element_iterator& operator--() noexcept(noexcept(--iter_)) {
        --iter_;
        return *this;
    }

    constexpr element_iterator operator--(int) noexcept(
        noexcept(--iter_) &&
        std::is_nothrow_copy_constructible_v<std::ranges::iterator_t<Rng>>) {
        auto temp = *this;
        --iter_;
        return temp;
    }

    constexpr element_iterator& operator+=(
        const difference_type offset) noexcept(noexcept(iter_ += offset))
    requires std::ranges::random_access_range<Rng>
    {
        iter_ += offset;
        return *this;
    }

    constexpr element_iterator operator+(const difference_type offset)
    requires std::ranges::random_access_range<Rng>
    {
        auto temp = *this;
        temp += offset;
        return temp;
    }

    constexpr element_iterator& operator-=(
        const difference_type offset) noexcept(noexcept(iter_ -= offset))
    requires std::ranges::random_access_range<Rng>
    {
        iter_ -= offset;
        return *this;
    }

    constexpr element_iterator operator-(const difference_type offset)
    requires std::ranges::random_access_range<Rng>
    {
        auto temp = *this;
        temp -= offset;
        return temp;
    }

    template <bool Const>
    constexpr bool operator==(
        const element_iterator<Rng, Index, Const>& that) const noexcept
    requires requires(iterator_type& lhs, iterator_type& rhs) { lhs == rhs; }
    {
        return iter_ == that.iter_;
    }

    template <bool Const>
    constexpr bool operator!=(
        const element_iterator<Rng, Index, Const>& that) const noexcept
    requires requires(iterator_type& lhs, iterator_type& rhs) { lhs != rhs; }
    {
        return iter_ != that.iter_;
    }

private:
    std::ranges::iterator_t<Rng> iter_;
};

/**
 * @brief Elements view, but supports user defined get.
 *
 * @tparam Vw
 * @tparam Index
 */
template <std::ranges::input_range Vw, size_t Index>
class elements_view : std::ranges::view_interface<elements_view<Vw, Index>> {
public:
    /**
     * @brief Constuct a elements view.
     *
     * @warning Move then constructing.
     */
    constexpr explicit elements_view(Vw range) noexcept(
        std::is_nothrow_constructible_v<Vw>)
        : range_(std::move(range)) {}

    constexpr auto begin() noexcept {
        return element_iterator<Vw, Index, false>(std::ranges::begin(range_));
    }

    constexpr auto begin() const noexcept {
        if constexpr (requires { std::ranges::cbegin(range_); }) {
            return element_iterator<Vw, Index, true>(
                std::ranges::cbegin(range_));
        } else {
            return element_iterator<Vw, Index, true>(
                std::ranges::begin(range_));
        }
    }

#if !HAS_CXX23
    constexpr auto cbegin() const noexcept {
        return element_iterator<Vw, Index, true>(std::ranges::cbegin(range_));
    }

    constexpr auto rbegin() noexcept {
        return element_iterator<Vw, Index, false>(std::ranges::rbegin(range_));
    }

    constexpr auto crbegin() const noexcept {
        return element_iterator<Vw, Index, true>(std::ranges::crbegin(range_));
    }
#endif

    constexpr auto end() noexcept {
        return element_iterator<Vw, Index, false>(std::ranges::end(range_));
    }

    constexpr auto end() const noexcept {
        if constexpr (requires { std::ranges::cend(range_); }) {
            return element_iterator<Vw, Index, true>(std::ranges::cend(range_));
        } else if constexpr (requires { std::end(range_); }) {
            return element_iterator<Vw, Index, true>(std::ranges::end(range_));
        } else {
            static_assert(false, "No suitable method to get a const iterator");
        }
    }

    constexpr auto cend() const noexcept { return std::ranges::cend(range_); }

    constexpr auto rend() noexcept { return std::ranges::rend(range_); }

    constexpr auto size() const noexcept
    requires std::ranges::sized_range<const Vw>
    {
        return std::ranges::size(range_);
    }

private:
    Vw range_;
};

template <size_t Index>
struct element_fn : range_adaptor_closure<element_fn<Index>> {

    template <std::ranges::viewable_range Rng>
    requires gettible<Index, std::ranges::range_value_t<Rng>>
    [[nodiscard]] constexpr auto operator()(Rng&& range) const noexcept {
        static_assert(
            Index < std::tuple_size_v<std::ranges::range_value_t<Rng>>,
            "Index should be in [0, size)");
        return elements_view<std::views::all_t<Rng>, Index>{ std::forward<Rng>(
            range) };
    }
};

template <size_t Index>
constexpr element_fn<Index> elements;

constexpr inline auto keys   = elements<0>;
constexpr inline auto values = elements<1>;

} // namespace neutron::ranges::views
