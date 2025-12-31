// IWYU pragma: private, include "neutron/utility.hpp"
#pragma once
#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include "neutron/detail/concepts/pair.hpp"
#include "neutron/detail/type_traits/same_cvref.hpp"
#include "neutron/detail/utility/compressed_element.hpp"

namespace neutron {

/**
 * @brief Compressed pair using EBCO (Empty Base Class Optimization)
 *
 * Memory-efficient pair implementation that uses inheritance for storage.
 * Supports structured bindings and standard pair operations.
 *
 * @tparam First Type of first element
 * @tparam Second Type of second element
 */
template <typename First, typename Second>
class compressed_pair :
    private _compressed_element<First, 0>,
    private _compressed_element<Second, 1> {
    using self_type   = compressed_pair;
    using first_base  = _compressed_element<First, 0>;
    using second_base = _compressed_element<Second, 1>;

public:
    using first_type  = First;
    using second_type = Second;

    /**
     * @brief Default constructor.
     *
     */
    constexpr compressed_pair() noexcept(
        std::is_nothrow_default_constructible_v<first_base> &&
        std::is_nothrow_default_constructible_v<second_base>)
        : first_base(), second_base() {}

    /**
     * @brief Construct each.
     *
     * @tparam FirstType
     * @tparam SecondType
     */
    template <typename FirstType, typename SecondType>
    constexpr compressed_pair(FirstType&& first, SecondType&& second) noexcept(
        std::is_nothrow_constructible_v<first_base, FirstType> &&
        std::is_nothrow_constructible_v<second_base, SecondType>)
        : first_base(std::forward<FirstType>(first)),
          second_base(std::forward<SecondType>(second)) {}

    template <typename Tuple1, typename Tuple2, size_t... Is1, size_t... Is2>
    constexpr compressed_pair(Tuple1& tup1, Tuple2& tup2, std::index_sequence<Is1...>, std::index_sequence<Is2...>) noexcept(
        std::is_nothrow_constructible_v<
            First, std::tuple_element_t<Is1, Tuple1>...> &&
        std::is_nothrow_constructible_v<
            Second, std::tuple_element_t<Is2, Tuple2>...>)
        : first_base(std::get<Is1>(std::move(tup1))...),
          second_base(std::get<Is2>(std::move(tup2))...) {}

    template <typename... Tys1, typename... Tys2>
    constexpr compressed_pair(std::piecewise_construct_t, std::tuple<Tys1...> tup1, std::tuple<Tys2...> tup2) noexcept(
        noexcept(compressed_pair(
            tup1, tup2, std::index_sequence_for<Tys1...>{},
            std::index_sequence_for<Tys2...>{})))
        : compressed_pair(
              tup1, tup2, std::index_sequence_for<Tys1...>{},
              std::index_sequence_for<Tys2...>{}) {}

    template <pair That>
    constexpr compressed_pair(That&& that) noexcept(
        std::is_nothrow_constructible_v<First, same_cvref_t<First, That>> &&
        std::is_nothrow_constructible_v<Second, same_cvref_t<Second, That>>)
    requires(!std::is_same_v<
                compressed_pair,
                std::remove_cv_t<std::remove_reference_t<That>>>)
        : first_base([&] {
              if constexpr (public_pair<That>) {
                  return std::forward<That>(that).first;
              } else if constexpr (private_pair<That>) {
                  return std::forward<That>(that).first();
              } else {
                  static_assert(
                      false, "no suitable way to construct the first element");
              }
          }),
          second_base([&] {
              if constexpr (public_pair<That>) {
                  return std::forward<That>(that).second;
              } else if constexpr (private_pair<That>) {
                  return std::forward<That>(that).second();
              } else {
                  static_assert(
                      false, "no suitable way to construct the second element");
              }
          }) {}

    constexpr compressed_pair(const compressed_pair&)            = default;
    constexpr compressed_pair& operator=(const compressed_pair&) = default;

    constexpr compressed_pair(compressed_pair&& that) noexcept(
        std::is_nothrow_move_constructible_v<first_base> &&
        std::is_nothrow_move_constructible_v<second_base>)
        // : first_base(std::move(static_cast<first_base&&>(that))),
        //   second_base(std::move(static_cast<second_base&&>(that))) {}
        : first_base(static_cast<first_base&&>(std::move(that))),
          second_base(static_cast<second_base&&>(std::move(that))) {}

    constexpr compressed_pair& operator=(compressed_pair&& that) noexcept(
        std::is_nothrow_move_assignable_v<first_base> &&
        std::is_nothrow_move_assignable_v<second_base>) {
        static_cast<first_base&>(*this) =
            static_cast<first_base&&>(std::move(that));
        static_cast<second_base&>(*this) =
            static_cast<second_base&&>(std::move(that));
        return *this;
    }

    template <typename Other1, typename Other2>
    requires std::convertible_to<Other1, First> &&
                 std::convertible_to<Other2, Second>
    constexpr compressed_pair(const compressed_pair<Other1, Other2>& that) noexcept(
        std::is_nothrow_constructible_v<First, const Other1&> &&
        std::is_nothrow_constructible_v<Second, const Other2&>)
        : first_base(that.first()), second_base(that.second()) {}

    template <typename Other1, typename Other2>
    requires std::convertible_to<Other1, First> &&
                 std::convertible_to<Other2, Second>
    constexpr compressed_pair(compressed_pair<Other1, Other2>&& that) noexcept(
        std::is_nothrow_constructible_v<First, Other1> &&
        std::is_nothrow_constructible_v<Second, Other2>)
        : first_base(std::forward<Other1>(std::move(that).first())),
          second_base(std::forward<Other2>(std::move(that).second())) {}

    constexpr ~compressed_pair() noexcept(
        std::is_nothrow_destructible_v<first_base> &&
        std::is_nothrow_destructible_v<second_base>) = default;

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

    template <size_t Index>
    requires(Index <= 1)
    ATOM_NODISCARD constexpr decltype(auto) get() & noexcept {
        if constexpr (Index == 0) {
            return first();
        } else {
            return second();
        }
    }

    template <size_t Index>
    requires(Index <= 1)
    ATOM_NODISCARD constexpr decltype(auto) get() const& noexcept {
        if constexpr (Index == 0) {
            return first();
        } else {
            return second();
        }
    }

    template <size_t Index>
    requires(Index <= 1)
    ATOM_NODISCARD constexpr decltype(auto) get() && noexcept {
        if constexpr (Index == 0) {
            return first();
        } else {
            return second();
        }
    }

    template <size_t Index>
    requires(Index <= 1)
    ATOM_NODISCARD constexpr decltype(auto) get() const&& noexcept {
        if constexpr (Index == 0) {
            return first();
        } else {
            return second();
        }
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
        return std::pair<First, Second>(first(), second());
    }

    template <
        template <typename, typename> typename PvtPair, typename F, typename S>
    requires private_pair<PvtPair<F, S>>
    ATOM_NODISCARD constexpr bool
        operator==(const PvtPair<F, S>& pair) const noexcept {
        if constexpr (std::is_same_v<First, F> && std::is_same_v<Second, S>) {
            return first() == pair.first() && second() == pair.second();
        }
        return false;
    }

    template <
        template <typename, typename> typename PvtPair, typename F, typename S>
    requires private_pair<PvtPair<F, S>>
    ATOM_NODISCARD constexpr bool
        operator!=(const PvtPair<F, S>& pair) const noexcept {
        if constexpr (std::is_same_v<First, F> && std::is_same_v<Second, S>) {
            return first() != pair.first() || second() != pair.second();
        }
        return true;
    }

    template <
        template <typename, typename> typename PubPair = std::pair,
        typename F = First, typename S = Second>
    requires public_pair<PubPair<F, S>>
    ATOM_NODISCARD constexpr bool
        operator==(const PubPair<F, S>& pair) const noexcept {
        if constexpr (std::is_same_v<F, F> && std::is_same_v<S, S>) {
            return first() == pair.first && second() == pair.second;
        }
        return false;
    }

    template <
        template <typename, typename> typename PubPair = std::pair,
        typename F = First, typename S = Second>
    requires public_pair<PubPair<F, S>>
    ATOM_NODISCARD constexpr bool
        operator!=(const PubPair<F, S>& pair) const noexcept {
        if constexpr (std::is_same_v<First, F> && std::is_same_v<Second, S>) {
            return first() != pair.first || second() != pair.second;
        }
        return true;
    }
};

template <typename First, typename Second>
compressed_pair(First, Second) -> compressed_pair<First, Second>;

template <typename First, typename Second>
constexpr auto make_compressed_pair(First&& first, Second&& second) noexcept(
    std::is_nothrow_constructible_v<std::decay_t<First>, First> &&
    std::is_nothrow_constructible_v<std::decay_t<Second>, Second>)
    -> compressed_pair<std::decay_t<First>, std::decay_t<Second>> {
    using pair = compressed_pair<std::decay_t<First>, std::decay_t<Second>>;
    return pair(std::forward<First>(first), std::forward<Second>(second));
}

} // namespace neutron

namespace std {

template <typename First, typename Second>
struct tuple_size<neutron::compressed_pair<First, Second>> :
    std::integral_constant<size_t, 2> {};

template <size_t Index, typename First, typename Second>
struct tuple_element<Index, neutron::compressed_pair<First, Second>> {
    static_assert(Index < 2);
    using pair = neutron::compressed_pair<First, Second>;
    using type = std::conditional_t<
        (Index == 0), typename pair::first_type, typename pair::second_type>;
};

} // namespace std
