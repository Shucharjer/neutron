#pragma once
#include <cstddef>
#include <type_traits>
#include <utility>
#include "neutron/detail/concepts/pair.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/type_traits/same_cv.hpp"
#include "neutron/detail/utility/compressed_element.hpp"
#include "neutron/detail/utility/compressed_pair.hpp"

namespace neutron {

/**
 * @brief Reversed compressed pair with element order swapped
 *
 * Storage layout matches compressed_pair but logical order is reversed
 */
template <typename First, typename Second>
class reversed_compressed_pair final :
    private _compressed_element<Second, 0>,
    private _compressed_element<First, 1> {
    using self_type   = reversed_compressed_pair;
    using first_base  = _compressed_element<Second, 0>;
    using second_base = _compressed_element<First, 1>;

public:
    using first_type  = First;
    using second_type = Second;

    constexpr reversed_compressed_pair() noexcept(
        std::is_nothrow_default_constructible_v<second_base> &&
        std::is_nothrow_default_constructible_v<first_base>)
        : first_base(), second_base() {}

    template <typename FirstType, typename SecondType>
    constexpr reversed_compressed_pair(FirstType&& first, SecondType&& second) noexcept(
        std::is_nothrow_constructible_v<second_base, SecondType> &&
        std::is_nothrow_constructible_v<first_base, FirstType>)
        : first_base(std::forward<SecondType>(second)),
          second_base(std::forward<FirstType>(first)) {}

    template <typename Tuple1, typename Tuple2, size_t... Is1, size_t... Is2>
    constexpr reversed_compressed_pair(Tuple1& tup1, Tuple2& tup2, std::index_sequence<Is1...>, std::index_sequence<Is2...>) noexcept(
        std::is_nothrow_constructible_v<
            First, std::tuple_element_t<Is1, Tuple1>...> &&
        std::is_nothrow_constructible_v<
            Second, std::tuple_element_t<Is2, Tuple2>...>)
        : first_base(std::get<Is1>(std::move(tup1))...),
          second_base(std::get<Is2>(std::move(tup2))...) {}

    template <typename... Tys1, typename... Tys2>
    constexpr reversed_compressed_pair(std::piecewise_construct_t, std::tuple<Tys1...> tup1, std::tuple<Tys2...> tup2) noexcept(
        noexcept(reversed_compressed_pair(
            tup1, tup2, std::index_sequence_for<Tys1...>{},
            std::index_sequence_for<Tys2...>{})))
        : reversed_compressed_pair(
              tup1, tup2, std::index_sequence_for<Tys1...>{},
              std::index_sequence_for<Tys2...>{}) {}

    constexpr reversed_compressed_pair(const reversed_compressed_pair&) =
        default;
    constexpr reversed_compressed_pair&
        operator=(const reversed_compressed_pair&) = default;

    constexpr reversed_compressed_pair(reversed_compressed_pair&& that) noexcept(
        std::is_nothrow_move_constructible_v<first_base> &&
        std::is_nothrow_move_constructible_v<second_base>)
        : first_base(std::move(static_cast<first_base&>(that))),
          second_base(std::move(static_cast<second_base&>(that))) {}

    constexpr reversed_compressed_pair&
        operator=(reversed_compressed_pair&& that) noexcept(
            std::is_nothrow_move_assignable_v<first_base> &&
            std::is_nothrow_move_assignable_v<second_base>) {
        first()  = std::move(that.first());
        second() = std::move(that.second());
        return *this;
    }

    constexpr ~reversed_compressed_pair() noexcept(
        std::is_nothrow_destructible_v<first_base> &&
        std::is_nothrow_destructible_v<second_base>) = default;

    ATOM_NODISCARD constexpr First& first() & noexcept {
        return static_cast<second_base&>(*this).value();
    }

    ATOM_NODISCARD constexpr const First& first() const& noexcept {
        return static_cast<const second_base&>(*this).value();
    }

    ATOM_NODISCARD constexpr First&& first() && noexcept {
        return static_cast<second_base&&>(*this).value();
    }

    ATOM_NODISCARD constexpr const First&& first() const&& noexcept {
        return static_cast<const second_base&&>(*this).value();
    }

    ATOM_NODISCARD constexpr Second& second() & noexcept {
        return static_cast<first_base&>(*this).value();
    }

    ATOM_NODISCARD constexpr const Second& second() const& noexcept {
        return static_cast<const first_base&>(*this).value();
    }

    ATOM_NODISCARD constexpr Second&& second() && noexcept {
        return static_cast<first_base&&>(*this).value();
    }

    ATOM_NODISCARD constexpr const Second&& second() const&& noexcept {
        return static_cast<const first_base&&>(*this).value();
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
        template <typename, typename> typename PubPair, typename F, typename S>
    requires public_pair<PubPair<F, S>>
    ATOM_NODISCARD constexpr bool
        operator==(const PubPair<F, S>& pair) const noexcept {
        if constexpr (std::is_same_v<F, F> && std::is_same_v<S, S>) {
            return first() == pair.first && second() == pair.second;
        }
        return false;
    }

    template <
        template <typename, typename> typename PubPair, typename F, typename S>
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
struct reversed_pair {
    using first_type  = Second;
    using second_type = First;

    first_type second;
    second_type first;

    template <
        template <typename, typename> typename PvtPair, typename F, typename S>
    requires private_pair<PvtPair<F, S>>
    ATOM_NODISCARD constexpr bool
        operator==(const PvtPair<F, S>& pair) const noexcept {
        if constexpr (std::is_same_v<First, F> && std::is_same_v<Second, S>) {
            return first == pair.first() && second == pair.second();
        }
        return false;
    }

    template <
        template <typename, typename> typename PvtPair, typename F, typename S>
    requires private_pair<PvtPair<F, S>>
    ATOM_NODISCARD constexpr bool
        operator!=(const PvtPair<F, S>& pair) const noexcept {
        if constexpr (std::is_same_v<First, F> && std::is_same_v<Second, S>) {
            return first != pair.first() || second != pair.second();
        }
        return true;
    }

    template <
        template <typename, typename> typename PubPair, typename F, typename S>
    requires public_pair<PubPair<F, S>>
    ATOM_NODISCARD constexpr bool
        operator==(const PubPair<F, S>& pair) const noexcept {
        if constexpr (std::is_same_v<F, F> && std::is_same_v<S, S>) {
            return first == pair.first && second == pair.second;
        }
        return false;
    }

    template <
        template <typename, typename> typename PubPair, typename F, typename S>
    requires public_pair<PubPair<F, S>>
    ATOM_NODISCARD constexpr bool
        operator!=(const PubPair<F, S>& pair) const noexcept {
        if constexpr (std::is_same_v<First, F> && std::is_same_v<Second, S>) {
            return first != pair.first || second != pair.second;
        }
        return true;
    }
};

template <
    typename First, typename Second,
    template <typename, typename> typename Pair = compressed_pair>
struct compated_pair {
public:
    using value_type  = Pair<First, Second>;
    using first_type  = typename value_type::first_type;
    using second_type = typename value_type::second_type;

    template <typename... Args>
    requires std::is_constructible_v<value_type, Args...>
    compated_pair(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<value_type, Args...>)
        : pair_(std::forward<Args>(args)...) {}

    compated_pair(const compated_pair& that) noexcept(
        std::is_nothrow_copy_constructible_v<value_type>)
        : pair_(that.pair_) {}

    compated_pair(compated_pair&& that) noexcept(
        std::is_nothrow_move_constructible_v<value_type>)
        : pair_(std::move(that.pair_)) {}

    template <typename... Tys1, typename... Tys2>
    compated_pair(
        std::piecewise_construct_t, std::tuple<Tys1...> tup1,
        std::tuple<Tys2...>
            tup2) noexcept(std::
                               is_nothrow_constructible_v<
                                   value_type, std::piecewise_construct_t,
                                   std::tuple<Tys1...>, std::tuple<Tys2...>>)
        : compated_pair(
              std::piecewise_construct, std::move(tup1), std::move(tup2)) {}

    compated_pair& operator=(const compated_pair& that) noexcept(
        std::is_nothrow_copy_assignable_v<value_type>) {
        if (this != &that) [[likely]] {
            pair_ = that.pair_;
        }
        return *this;
    }

    compated_pair& operator=(compated_pair&& that) noexcept(
        std::is_nothrow_move_assignable_v<value_type>) {
        if (this != &that) [[likely]] {
            pair_ = that.pair_;
        }
        return *this;
    }

    ~compated_pair() noexcept(std::is_nothrow_destructible_v<value_type>) =
        default;

    constexpr First& first() & noexcept {
        if constexpr (public_pair<value_type>) {
            return pair_.first;
        } else if constexpr (private_pair<value_type>) {
            return pair_.first();
        } else {
            static_assert(false, "No valid way to get the first value.");
        }
    }

    constexpr const First& first() const& noexcept {
        if constexpr (public_pair<value_type>) {
            return pair_.first;
        } else if constexpr (private_pair<value_type>) {
            return pair_.first();
        } else {
            static_assert(false, "No valid way to get the first value.");
        }
    }

    constexpr First&& first() && noexcept {
        if constexpr (public_pair<value_type>) {
            return pair_.first;
        } else if constexpr (private_pair<value_type>) {
            return pair_.first();
        } else {
            static_assert(false, "No valid way to get the first value.");
        }
    }

    constexpr const First&& first() const&& noexcept {
        if constexpr (public_pair<value_type>) {
            return pair_.first;
        } else if constexpr (private_pair<value_type>) {
            return pair_.first();
        } else {
            static_assert(false, "No valid way to get the first value.");
        }
    }

    constexpr Second& second() & noexcept {
        if constexpr (public_pair<value_type>) {
            return pair_.second;
        } else if constexpr (private_pair<value_type>) {
            return pair_.second();
        } else {
            static_assert(false, "No valid way to get the second value.");
        }
    }

    constexpr const Second& second() const& noexcept {
        if constexpr (public_pair<value_type>) {
            return pair_.second;
        } else if constexpr (private_pair<value_type>) {
            return pair_.second();
        } else {
            static_assert(false, "No valid way to get the second value.");
        }
    }

    constexpr Second&& second() && noexcept {
        if constexpr (public_pair<value_type>) {
            return pair_.second;
        } else if constexpr (private_pair<value_type>) {
            return pair_.second();
        } else {
            static_assert(false, "No valid way to get the second value.");
        }
    }

    constexpr const Second&& second() const&& noexcept {
        if constexpr (public_pair<value_type>) {
            return pair_.second;
        } else if constexpr (private_pair<value_type>) {
            return pair_.second();
        } else {
            static_assert(false, "No valid way to get the second value.");
        }
    }

    constexpr bool operator==(const compated_pair& that) const noexcept {
        return pair_ == that.pair_;
    }

    template <typename Ty>
    constexpr bool operator==(const Ty& that) const noexcept {
        if constexpr (requires { pair_ == that; }) {
            return pair_ == that;
        } else {
            return false;
        }
    }

    constexpr bool operator!=(const compated_pair& that) const noexcept {
        return pair_ != that.pair_;
    }

    template <typename Ty>
    constexpr bool operator!=(const Ty& that) const noexcept {
        if constexpr (requires { pair_ != that; }) {
            return pair_ != that;
        } else {
            return false;
        }
    }

    template <size_t Index>
    requires(Index <= 1)
    ATOM_NODISCARD constexpr auto& get() & noexcept {
        if constexpr (Index == 0) {
            return first();
        } else {
            return second();
        }
    }

    template <size_t Index>
    requires(Index <= 1)
    ATOM_NODISCARD constexpr auto& get() const& noexcept {
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

private:
    Pair<First, Second> pair_;
};

template <typename>
struct reversed_result;

template <typename First, typename Second>
struct reversed_result<compressed_pair<First, Second>> {
    using type = reversed_compressed_pair<Second, First>;
};

template <typename First, typename Second>
struct reversed_result<reversed_compressed_pair<First, Second>> {
    using type = compressed_pair<Second, First>;
};

template <typename First, typename Second>
struct reversed_result<std::pair<First, Second>> {
    using type = reversed_pair<Second, First>;
};

template <typename First, typename Second>
struct reversed_result<reversed_pair<Second, First>> {
    using type = std::pair<Second, First>;
};

template <
    typename First, typename Second,
    template <typename, typename> typename Pair>
struct reversed_result<compated_pair<First, Second, Pair>> {
    using type = compated_pair<Second, First, Pair>;
};

template <typename Pair>
using reversed_result_t = typename reversed_result<Pair>::type;

/*! @cond TURN_OFF_DOXYGEN */
template <typename Ty>
concept _reversible_pair = pair<Ty> && requires {
    typename reversed_result<std::remove_cvref_t<Ty>>::type;
    std::is_trivial_v<typename std::remove_cvref_t<Ty>::first_type>;
    std::is_trivial_v<typename std::remove_cvref_t<Ty>::second_type>;
};
/*! @endcond */

/**
 * @brief Get the reversed pair.
 *
 * @tparam Pair The pair type. This tparam could be deduced automatically.
 * @param pair The pair need to reverse.
 * @return Reversed pair.
 */
template <_reversible_pair Pair>
constexpr decltype(auto) reverse(Pair& pair) noexcept {
    using result_type = typename ::neutron::same_cv_t<
        reversed_result_t<std::remove_cv_t<Pair>>, Pair>;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<result_type&>(pair);
}

} // namespace neutron

/*! @cond TURN_OFF_DOXYGEN */

namespace std {

template <typename First, typename Second>
struct tuple_size<neutron::reversed_compressed_pair<First, Second>> :
    std::integral_constant<size_t, 2> {};

template <
    typename First, typename Second,
    template <typename, typename> typename Pair>
struct tuple_size<neutron::compated_pair<First, Second, Pair>> :
    std::integral_constant<size_t, 2> {};

template <size_t Index, typename First, typename Second>
struct tuple_element<Index, neutron::reversed_compressed_pair<First, Second>> {
    static_assert(Index < 2);
    using pair = neutron::reversed_compressed_pair<First, Second>;
    using type = std::conditional_t<
        (Index == 0), typename pair::first_type, typename pair::second_type>;
};

template <
    size_t Index, typename First, typename Second,
    template <typename, typename> typename Pair>
struct tuple_element<Index, neutron::compated_pair<First, Second, Pair>> {
    static_assert(Index < 2);
    using pair = neutron::compated_pair<First, Second, Pair>;
    using type = std::conditional_t<
        (Index == 0), typename pair::first_type, typename pair::second_type>;
};

} // namespace std

/*! @endcond */
