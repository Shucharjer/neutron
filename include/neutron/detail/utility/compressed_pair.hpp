// IWYU pragma: private, include <neutron/utility.hpp>
#pragma once
#include <compare>
#include <concepts>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include "neutron/detail/concepts/pair.hpp"
#include "neutron/detail/utility/compressed_element.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */
namespace _compressed_pair {

template <typename Ty>
void _implicit_test(const Ty&);

template <typename Ty>
concept implicitly_default_constructible = requires { _implicit_test<Ty>({}); };

/// Pair types accepted as sources: anything with `first`/`second` members or
/// `first()`/`second()` accessors.
template <typename Pair>
concept pair_source = pair<std::remove_cvref_t<Pair>>;

template <typename Pair>
requires public_pair<std::remove_cvref_t<Pair>>
constexpr decltype(auto) first_of(Pair&& pair) noexcept {
    return (std::forward<Pair>(pair).first);
}

template <typename Pair>
requires(!public_pair<std::remove_cvref_t<Pair>>) &&
        private_pair<std::remove_cvref_t<Pair>>
constexpr decltype(auto)
    first_of(Pair&& pair) noexcept(noexcept(std::forward<Pair>(pair).first())) {
    return std::forward<Pair>(pair).first();
}

template <typename Pair>
requires public_pair<std::remove_cvref_t<Pair>>
constexpr decltype(auto) second_of(Pair&& pair) noexcept {
    return (std::forward<Pair>(pair).second);
}

template <typename Pair>
requires(!public_pair<std::remove_cvref_t<Pair>>) &&
        private_pair<std::remove_cvref_t<Pair>>
constexpr decltype(auto) second_of(Pair&& pair) noexcept(
    noexcept(std::forward<Pair>(pair).second())) {
    return std::forward<Pair>(pair).second();
}

template <typename Pair>
using first_t = decltype(first_of(std::declval<Pair>()));

template <typename Pair>
using second_t = decltype(second_of(std::declval<Pair>()));

/// The synth-three-way comparison used by `std::pair`.
struct synth_three_way_fn {
    template <typename Lhs, typename Rhs>
    requires requires(const Lhs& lhs, const Rhs& rhs) {
        { lhs < rhs } -> std::convertible_to<bool>;
        { rhs < lhs } -> std::convertible_to<bool>;
    }
    constexpr auto operator()(const Lhs& lhs, const Rhs& rhs) const {
        if constexpr (std::three_way_comparable_with<Lhs, Rhs>) {
            return lhs <=> rhs;
        } else {
            if (lhs < rhs) {
                return std::weak_ordering::less;
            }
            if (rhs < lhs) {
                return std::weak_ordering::greater;
            }
            return std::weak_ordering::equivalent;
        }
    }
};

inline constexpr synth_three_way_fn synth_three_way{};

template <typename Lhs, typename Rhs>
using synth_three_way_result = decltype(synth_three_way(
    std::declval<const Lhs&>(), std::declval<const Rhs&>()));

template <typename Pair, typename First, typename Second>
concept constructible_from =
    pair_source<Pair> && std::is_constructible_v<First, first_t<Pair>> &&
    std::is_constructible_v<Second, second_t<Pair>>;

template <typename Pair, typename First, typename Second>
concept convertible_from = std::is_convertible_v<first_t<Pair>, First> &&
                           std::is_convertible_v<second_t<Pair>, Second>;

template <typename Pair, typename First, typename Second>
concept assignable_from =
    pair_source<Pair> && std::is_assignable_v<First&, first_t<Pair>> &&
    std::is_assignable_v<Second&, second_t<Pair>>;

template <typename Pair, typename First, typename Second>
inline constexpr bool nothrow_constructible_from =
    noexcept(first_of(std::declval<Pair>())) &&
    noexcept(second_of(std::declval<Pair>())) &&
    std::is_nothrow_constructible_v<First, first_t<Pair>> &&
    std::is_nothrow_constructible_v<Second, second_t<Pair>>;

struct piecewise_tag {};

} // namespace _compressed_pair
/*! @endcond */

/**
 * @brief A pair that takes no space for empty element types.
 *
 * Behaves like `std::pair`, except that the elements are accessed through
 * `first()` and `second()`. Empty element types occupy no storage, and the
 * pair is trivially copyable whenever both element types are.
 *
 * Copy and move assignment are defaulted, so a pair holding a reference is
 * not assignable, unlike `std::pair`.
 *
 * @tparam First Type of the first element.
 * @tparam Second Type of the second element.
 */
template <typename First, typename Second>
class ATOM_EMPTY_BASES compressed_pair :
    private _compressed_element<First, 0>,
    private _compressed_element<Second, 1> {
    using first_base  = _compressed_element<First, 0>;
    using second_base = _compressed_element<Second, 1>;

    template <typename Pair>
    static constexpr bool _is_self =
        std::is_same_v<std::remove_cvref_t<Pair>, compressed_pair>;

    template <size_t Index, typename Self>
    static constexpr decltype(auto) _get(Self&& self) noexcept {
        if constexpr (Index == 0) {
            return std::forward<Self>(self).first();
        } else {
            return std::forward<Self>(self).second();
        }
    }

    template <typename Tuple1, typename Tuple2, size_t... Is1, size_t... Is2>
    constexpr compressed_pair(
        _compressed_pair::piecewise_tag, Tuple1& tup1, Tuple2& tup2,
        std::index_sequence<Is1...>, std::index_sequence<Is2...>)
        : first_base(std::in_place, std::get<Is1>(std::move(tup1))...),
          second_base(std::in_place, std::get<Is2>(std::move(tup2))...) {}

public:
    using first_type  = First;
    using second_type = Second;

    /**
     * @brief Value-initializes both elements.
     */
    constexpr explicit(
        !_compressed_pair::implicitly_default_constructible<First> ||
        !_compressed_pair::implicitly_default_constructible<Second>)
        compressed_pair() noexcept(
            std::is_nothrow_default_constructible_v<First> &&
            std::is_nothrow_default_constructible_v<Second>)
    requires std::is_default_constructible_v<First> &&
                 std::is_default_constructible_v<Second>
        : first_base(), second_base() {}

    constexpr explicit(
        !std::is_convertible_v<const First&, First> ||
        !std::is_convertible_v<const Second&, Second>)
        compressed_pair(const First& first, const Second& second) noexcept(
            std::is_nothrow_copy_constructible_v<First> &&
            std::is_nothrow_copy_constructible_v<Second>)
    requires std::is_copy_constructible_v<First> &&
                 std::is_copy_constructible_v<Second>
        : first_base(std::in_place, first), second_base(std::in_place, second) {
    }

    template <typename Ty1 = First, typename Ty2 = Second>
    requires std::is_constructible_v<First, Ty1> &&
                 std::is_constructible_v<Second, Ty2>
    constexpr explicit(
        !std::is_convertible_v<Ty1, First> ||
        !std::is_convertible_v<Ty2, Second>)
        compressed_pair(Ty1&& first, Ty2&& second) noexcept(
            std::is_nothrow_constructible_v<First, Ty1> &&
            std::is_nothrow_constructible_v<Second, Ty2>)
        : first_base(std::in_place, std::forward<Ty1>(first)),
          second_base(std::in_place, std::forward<Ty2>(second)) {}

    /**
     * @brief Converts from another pair, such as `std::pair` or a
     * `compressed_pair` of other types. The value category of `pair` is
     * propagated to its elements.
     */
    template <typename Pair>
    requires(!_is_self<Pair>) &&
                _compressed_pair::constructible_from<Pair, First, Second>
    constexpr explicit(!_compressed_pair::convertible_from<Pair, First, Second>)
        compressed_pair(Pair&& pair) noexcept(
            _compressed_pair::nothrow_constructible_from<Pair, First, Second>)
        : first_base(
              std::in_place,
              _compressed_pair::first_of(std::forward<Pair>(pair))),
          second_base(
              std::in_place,
              _compressed_pair::second_of(std::forward<Pair>(pair))) {}

    template <typename... Args1, typename... Args2>
    requires std::is_constructible_v<First, Args1...> &&
             std::is_constructible_v<Second, Args2...>
    constexpr compressed_pair(std::piecewise_construct_t, std::tuple<Args1...> first_args, std::tuple<Args2...> second_args) noexcept(
        std::is_nothrow_constructible_v<First, Args1...> &&
        std::is_nothrow_constructible_v<Second, Args2...>)
        : compressed_pair(
              _compressed_pair::piecewise_tag{}, first_args, second_args,
              std::index_sequence_for<Args1...>{},
              std::index_sequence_for<Args2...>{}) {}

    constexpr compressed_pair(const compressed_pair&)            = default;
    constexpr compressed_pair(compressed_pair&&)                 = default;
    constexpr compressed_pair& operator=(const compressed_pair&) = default;
    constexpr compressed_pair& operator=(compressed_pair&&)      = default;

    /**
     * @brief Assigns from another pair, propagating its value category.
     */
    template <typename Pair>
    requires(!_is_self<Pair>) &&
            _compressed_pair::assignable_from<Pair, First, Second>
    constexpr compressed_pair& operator=(Pair&& pair) noexcept(
        std::is_nothrow_assignable_v<First&, _compressed_pair::first_t<Pair>> &&
        std::is_nothrow_assignable_v<
            Second&, _compressed_pair::second_t<Pair>>) {
        first()  = _compressed_pair::first_of(std::forward<Pair>(pair));
        second() = _compressed_pair::second_of(std::forward<Pair>(pair));
        return *this;
    }

    constexpr void swap(compressed_pair& that) noexcept(
        std::is_nothrow_swappable_v<First> &&
        std::is_nothrow_swappable_v<Second>)
    requires std::is_swappable_v<First> && std::is_swappable_v<Second>
    {
        using std::swap;
        swap(first(), that.first());
        swap(second(), that.second());
    }

    friend constexpr void swap(
        compressed_pair& lhs,
        compressed_pair& rhs) noexcept(noexcept(lhs.swap(rhs)))
    requires std::is_swappable_v<First> && std::is_swappable_v<Second>
    {
        lhs.swap(rhs);
    }

    ATOM_NODISCARD constexpr First& first() & noexcept {
        return static_cast<first_base&>(*this).value();
    }

    ATOM_NODISCARD constexpr const First& first() const& noexcept {
        return static_cast<const first_base&>(*this).value();
    }

    ATOM_NODISCARD constexpr First&& first() && noexcept {
        return static_cast<first_base&&>(*this).value();
    }

    ATOM_NODISCARD constexpr const First&& first() const&& noexcept {
        return static_cast<const first_base&&>(*this).value();
    }

    ATOM_NODISCARD constexpr Second& second() & noexcept {
        return static_cast<second_base&>(*this).value();
    }

    ATOM_NODISCARD constexpr const Second& second() const& noexcept {
        return static_cast<const second_base&>(*this).value();
    }

    ATOM_NODISCARD constexpr Second&& second() && noexcept {
        return static_cast<second_base&&>(*this).value();
    }

    ATOM_NODISCARD constexpr const Second&& second() const&& noexcept {
        return static_cast<const second_base&&>(*this).value();
    }

    /**
     * @brief Gets an element, as `std::get` does for `std::pair`. Also used by
     * structured bindings.
     */
    template <size_t Index>
    requires(Index < 2)
    ATOM_NODISCARD constexpr decltype(auto) get() & noexcept {
        return _get<Index>(*this);
    }

    template <size_t Index>
    requires(Index < 2)
    ATOM_NODISCARD constexpr decltype(auto) get() const& noexcept {
        return _get<Index>(*this);
    }

    template <size_t Index>
    requires(Index < 2)
    ATOM_NODISCARD constexpr decltype(auto) get() && noexcept {
        return _get<Index>(std::move(*this));
    }

    template <size_t Index>
    requires(Index < 2)
    ATOM_NODISCARD constexpr decltype(auto) get() const&& noexcept {
        return _get<Index>(std::move(*this));
    }

    constexpr operator std::pair<First, Second>() const& noexcept(
        std::is_nothrow_copy_constructible_v<First> &&
        std::is_nothrow_copy_constructible_v<Second>)
    requires std::is_copy_constructible_v<First> &&
             std::is_copy_constructible_v<Second>
    {
        return std::pair<First, Second>(first(), second());
    }

    constexpr operator std::pair<First, Second>() && noexcept(
        std::is_nothrow_move_constructible_v<First> &&
        std::is_nothrow_move_constructible_v<Second>)
    requires std::is_move_constructible_v<First> &&
             std::is_move_constructible_v<Second>
    {
        return std::pair<First, Second>(
            std::move(*this).first(), std::move(*this).second());
    }

    /**
     * @brief Compares element-wise with any pair, in either order.
     */
    template <_compressed_pair::pair_source Pair>
    requires requires(const compressed_pair& lhs, const Pair& rhs) {
        {
            lhs.first() == _compressed_pair::first_of(rhs)
        } -> std::convertible_to<bool>;
        {
            lhs.second() == _compressed_pair::second_of(rhs)
        } -> std::convertible_to<bool>;
    }
    ATOM_NODISCARD friend constexpr bool
        operator==(const compressed_pair& lhs, const Pair& rhs) {
        return lhs.first() == _compressed_pair::first_of(rhs) &&
               lhs.second() == _compressed_pair::second_of(rhs);
    }

    /**
     * @brief Compares lexicographically with any pair, in either order.
     */
    template <_compressed_pair::pair_source Pair>
    requires requires(const compressed_pair& lhs, const Pair& rhs) {
        _compressed_pair::synth_three_way(
            lhs.first(), _compressed_pair::first_of(rhs));
        _compressed_pair::synth_three_way(
            lhs.second(), _compressed_pair::second_of(rhs));
    }
    ATOM_NODISCARD friend constexpr auto operator<=>(
        const compressed_pair& lhs, const Pair& rhs)
        -> std::common_comparison_category_t<
            _compressed_pair::synth_three_way_result<
                First,
                std::remove_cvref_t<_compressed_pair::first_t<const Pair&>>>,
            _compressed_pair::synth_three_way_result<
                Second,
                std::remove_cvref_t<_compressed_pair::second_t<const Pair&>>>> {
        if (auto result = _compressed_pair::synth_three_way(
                lhs.first(), _compressed_pair::first_of(rhs));
            result != 0) {
            return result;
        }
        return _compressed_pair::synth_three_way(
            lhs.second(), _compressed_pair::second_of(rhs));
    }
};

template <typename First, typename Second>
compressed_pair(First, Second) -> compressed_pair<First, Second>;

/**
 * @brief Creates a `compressed_pair`, unwrapping `std::reference_wrapper` like
 * `std::make_pair`.
 */
template <typename First, typename Second>
ATOM_NODISCARD constexpr auto
    make_compressed_pair(First&& first, Second&& second) noexcept(
        std::is_nothrow_constructible_v<
            std::unwrap_ref_decay_t<First>, First> &&
        std::is_nothrow_constructible_v<
            std::unwrap_ref_decay_t<Second>, Second>)
        -> compressed_pair<
            std::unwrap_ref_decay_t<First>, std::unwrap_ref_decay_t<Second>> {
    return compressed_pair<
        std::unwrap_ref_decay_t<First>, std::unwrap_ref_decay_t<Second>>(
        std::forward<First>(first), std::forward<Second>(second));
}

} // namespace neutron

namespace std {

template <typename First, typename Second>
struct tuple_size<neutron::compressed_pair<First, Second>> :
    std::integral_constant<size_t, 2> {};

template <size_t Index, typename First, typename Second>
struct tuple_element<Index, neutron::compressed_pair<First, Second>> {
    static_assert(Index < 2, "compressed_pair index out of range");
    using type = std::conditional_t<Index == 0, First, Second>;
};

} // namespace std
