#pragma once
#include <cstddef>
#include <type_traits>
#include <utility>
#include "neutron/type_traits.hpp"
#include "../src/neutron/internal/macros.hpp"
#include "../src/neutron/internal/utility/compressed_element.hpp"

namespace neutron {

/**
 * @brief Concept for public member pair types
 *
 * Requires first/second members and first_type/second_type typedefs
 */
template <typename Ty>
concept _public_pair = requires(const Ty& val) {
    typename Ty::first_type;
    typename Ty::second_type;
    val.first;
    val.second;
    requires std::is_member_object_pointer_v<decltype(&Ty::first)>;
    requires std::is_member_object_pointer_v<decltype(&Ty::second)>;
};

/**
 * @brief Concept for private member pair types
 *
 * Requires first()/second() methods and first_type/second_type typedefs
 */
template <typename Ty>
concept _private_pair = requires(const Ty& val) {
    typename Ty::first_type;
    typename Ty::second_type;
    val.first();
    val.second();
};

/**
 * @brief General pair concept
 *
 * Matches either public or private pair types
 */
template <typename Ty>
concept _pair = _public_pair<Ty> || _private_pair<Ty>;

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
class compressed_pair final :
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

    template <typename That>
    constexpr compressed_pair(That&& that) noexcept(
        std::is_nothrow_constructible_v<First, same_reference_t<First, That>> &&
        std::is_nothrow_constructible_v<Second, same_reference_t<Second, That>>)
    requires(!std::is_same_v<
                compressed_pair,
                std::remove_cv_t<std::remove_reference_t<That>>>)
        : first_base([&] {
              if constexpr (_public_pair<That>) {
                  return std::forward<That>(that).first;
              } else if constexpr (_private_pair<That>) {
                  return std::forward<That>(that).first();
              } else {
                  static_assert(
                      false, "no suitable way to construct the first element");
              }
          }),
          second_base([&] {
              if constexpr (_public_pair<That>) {
                  return std::forward<That>(that).second;
              } else if constexpr (_private_pair<That>) {
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

    constexpr ~compressed_pair() noexcept(
        std::is_nothrow_destructible_v<first_base> &&
        std::is_nothrow_destructible_v<second_base>) = default;

    NODISCARD constexpr First& first() & noexcept {
        return static_cast<first_base&>(*this).value();
    }

    NODISCARD constexpr const First& first() const& noexcept {
        return static_cast<const first_base&>(*this).value();
    }

    NODISCARD constexpr First&& first() && noexcept {
        return static_cast<first_base&&>(*this).value();
    }

    NODISCARD constexpr Second& second() & noexcept {
        return static_cast<second_base&>(*this).value();
    }

    NODISCARD constexpr const Second& second() const& noexcept {
        return static_cast<const second_base&>(*this).value();
    }

    NODISCARD constexpr Second&& second() && noexcept {
        return static_cast<second_base&&>(*this).value();
    }

    template <size_t Index>
    requires(Index <= 1)
    NODISCARD constexpr decltype(auto) get() & noexcept {
        if constexpr (Index == 0) {
            return first();
        } else {
            return second();
        }
    }

    template <size_t Index>
    requires(Index <= 1)
    NODISCARD constexpr decltype(auto) get() const& noexcept {
        if constexpr (Index == 0) {
            return first();
        } else {
            return second();
        }
    }

    template <size_t Index>
    requires(Index <= 1)
    NODISCARD constexpr decltype(auto) get() && noexcept {
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
};

// if it returns true, two `compressed_pair`s must have the same tparams.
template <typename LFirst, typename LSecond, typename RFirst, typename RSecond>
NODISCARD constexpr bool operator==(
    const compressed_pair<LFirst, LSecond>& lhs,
    const compressed_pair<RFirst, RSecond>& rhs) noexcept {
    if constexpr (
        std::is_same_v<LFirst, RFirst> && std::is_same_v<LSecond, RSecond>) {
        return lhs.first() == rhs.first() && lhs.second() && rhs.second();
    } else {
        return false;
    }
}

template <typename LFirst, typename LSecond, typename RFirst, typename RSecond>
NODISCARD constexpr bool operator!=(
    const compressed_pair<LFirst, LSecond>& lhs,
    const compressed_pair<RFirst, RSecond>& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename Ty, typename First, typename Second>
NODISCARD constexpr bool operator==(
    const compressed_pair<First, Second>& pair, const Ty& val) noexcept {
    if constexpr (std::is_convertible_v<Ty, First>) {
        return pair.first() == val;
    } else if constexpr (std::is_convertible_v<Ty, Second>) {
        return pair.second() == val;
    } else {
        return false;
    }
}

template <typename Ty, typename First, typename Second>
NODISCARD constexpr bool operator!=(
    const compressed_pair<First, Second>& pair, const Ty& val) noexcept {
    return !(pair == val);
}

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

    NODISCARD constexpr First& first() & noexcept {
        return static_cast<second_base&>(*this).value();
    }

    NODISCARD constexpr const First& first() const& noexcept {
        return static_cast<const second_base&>(*this).value();
    }

    NODISCARD constexpr First&& first() && noexcept {
        return static_cast<const second_base&>(*this).value();
    }

    NODISCARD constexpr Second& second() & noexcept {
        return static_cast<first_base&>(*this).value();
    }

    NODISCARD constexpr const Second& second() const& noexcept {
        return static_cast<const first_base&>(*this).value();
    }

    NODISCARD constexpr Second& second() && noexcept {
        return static_cast<first_base&>(*this).value();
    }

    template <size_t Index>
    requires(Index <= 1)
    NODISCARD constexpr decltype(auto) get() & noexcept {
        if constexpr (Index == 0) {
            return first();
        } else {
            return second();
        }
    }

    template <size_t Index>
    requires(Index <= 1)
    NODISCARD constexpr decltype(auto) get() const& noexcept {
        if constexpr (Index == 0) {
            return first();
        } else {
            return second();
        }
    }

    template <size_t Index>
    requires(Index <= 1)
    NODISCARD constexpr decltype(auto) get() && noexcept {
        if constexpr (Index == 0) {
            return first();
        } else {
            return second();
        }
    }
};

// if it returns true, two `reversed_compressed_pair`s must have the same
// tparams.
template <typename LFirst, typename LSecond, typename RFirst, typename RSecond>
constexpr bool operator==(
    const reversed_compressed_pair<LFirst, LSecond>& lhs,
    const reversed_compressed_pair<RFirst, RSecond>& rhs) {
    if constexpr (
        std::is_same_v<LFirst, RFirst> && std::is_same_v<LFirst, RFirst>) {
        return lhs.first() == rhs.first();
    } else {
        return false;
    }
}

template <typename LFirst, typename LSecond, typename RFirst, typename RSecond>
constexpr bool operator!=(
    const reversed_compressed_pair<LFirst, LSecond>& lhs,
    const reversed_compressed_pair<RFirst, RSecond>& rhs) {
    return !(lhs == rhs);
}

template <typename Ty, typename First, typename Second>
constexpr bool operator==(
    const Ty& lhs, const reversed_compressed_pair<First, Second>& rhs) {
    if constexpr (std::is_convertible_v<Ty, First>) {
        return lhs == rhs.first();
    } else if constexpr (std::is_convertible_v<Ty, Second>) {
        return lhs == rhs.second();
    } else {
        return false;
    }
}

template <typename Ty, typename First, typename Second>
constexpr bool operator!=(
    const Ty& lhs, const reversed_compressed_pair<First, Second>& rhs) {
    return !(lhs == rhs);
}

template <typename First, typename Second>
struct reversed_pair {
    using first_type  = Second;
    using second_type = First;

    first_type second;
    second_type first;
};

template <typename LFirst, typename LSecond, typename RFirst, typename RSecond>
constexpr bool operator==(
    const reversed_pair<LFirst, LSecond>& lhs,
    const reversed_pair<RFirst, RSecond>& rhs) {
    if constexpr (
        std::is_same_v<LFirst, RFirst> && std::is_same_v<LSecond, RSecond>) {
        return lhs.second == rhs.second && lhs.first == rhs.first;
    } else {
        return false;
    }
}

template <typename LFirst, typename LSecond, typename RFirst, typename RSecond>
constexpr bool operator!=(
    const reversed_pair<LFirst, LSecond>& lhs,
    const reversed_pair<RFirst, RSecond>& rhs) {
    return !(lhs == rhs);
}

template <typename Ty, typename First, typename Second>
constexpr bool
    operator==(const Ty& lhs, const reversed_pair<First, Second>& rhs) {
    if constexpr (std::is_convertible_v<Ty, First>) {
        return rhs.first == lhs;
    } else if constexpr (std::is_convertible_v<Ty, Second>) {
        return rhs.second == lhs;
    } else {
        return false;
    }
}

template <typename Ty, typename First, typename Second>
constexpr bool
    operator!=(const Ty& lhs, const reversed_pair<First, Second>& rhs) {
    return !(lhs == rhs);
}

template <
    typename First, typename Second,
    template <typename, typename> typename Pair = compressed_pair>
struct pair {
public:
    using value_type  = Pair<First, Second>;
    using first_type  = typename value_type::first_type;
    using second_type = typename value_type::second_type;

    template <typename... Args>
    requires std::is_constructible_v<value_type, Args...>
    pair(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<value_type, Args...>)
        : pair_(std::forward<Args>(args)...) {}

    pair(const pair& that) noexcept(
        std::is_nothrow_copy_constructible_v<value_type>)
        : pair_(that.pair_) {}

    pair(pair&& that) noexcept(std::is_nothrow_move_constructible_v<value_type>)
        : pair_(std::move(that.pair_)) {}

    template <typename... Tys1, typename... Tys2>
    pair(
        std::piecewise_construct_t, std::tuple<Tys1...> tup1,
        std::tuple<Tys2...>
            tup2) noexcept(std::
                               is_nothrow_constructible_v<
                                   value_type, std::piecewise_construct_t,
                                   std::tuple<Tys1...>, std::tuple<Tys2...>>)
        : pair(std::piecewise_construct, std::move(tup1), std::move(tup2)) {}

    pair& operator=(const pair& that) noexcept(
        std::is_nothrow_copy_assignable_v<value_type>) {
        if (this != &that) [[likely]] {
            pair_ = that.pair_;
        }
        return *this;
    }

    pair& operator=(pair&& that) noexcept(
        std::is_nothrow_move_assignable_v<value_type>) {
        if (this != &that) [[likely]] {
            pair_ = that.pair_;
        }
        return *this;
    }

    ~pair() noexcept(std::is_nothrow_destructible_v<value_type>) = default;

    constexpr First& first() & noexcept {
        if constexpr (_public_pair<value_type>) {
            return pair_.first;
        } else if constexpr (_private_pair<value_type>) {
            return pair_.first();
        } else {
            static_assert(false, "No valid way to get the first value.");
        }
    }

    constexpr const First& first() const& noexcept {
        if constexpr (_public_pair<value_type>) {
            return pair_.first;
        } else if constexpr (_private_pair<value_type>) {
            return pair_.first();
        } else {
            static_assert(false, "No valid way to get the first value.");
        }
    }

    constexpr First&& first() && noexcept {
        if constexpr (_public_pair<value_type>) {
            return pair_.first;
        } else if constexpr (_private_pair<value_type>) {
            return pair_.first();
        } else {
            static_assert(false, "No valid way to get the first value.");
        }
    }

    constexpr Second& second() & noexcept {
        if constexpr (_public_pair<value_type>) {
            return pair_.second;
        } else if constexpr (_private_pair<value_type>) {
            return pair_.second();
        } else {
            static_assert(false, "No valid way to get the second value.");
        }
    }

    constexpr const Second& second() const& noexcept {
        if constexpr (_public_pair<value_type>) {
            return pair_.second;
        } else if constexpr (_private_pair<value_type>) {
            return pair_.second();
        } else {
            static_assert(false, "No valid way to get the second value.");
        }
    }

    constexpr Second&& second() && noexcept {
        if constexpr (_public_pair<value_type>) {
            return pair_.second;
        } else if constexpr (_private_pair<value_type>) {
            return pair_.second();
        } else {
            static_assert(false, "No valid way to get the second value.");
        }
    }

    constexpr bool operator==(const pair& that) const noexcept {
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

    constexpr bool operator!=(const pair& that) const noexcept {
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
    NODISCARD constexpr auto& get() & noexcept {
        if constexpr (Index == 0) {
            return first();
        } else {
            return second();
        }
    }

    template <size_t Index>
    requires(Index <= 1)
    NODISCARD constexpr auto& get() const& noexcept {
        if constexpr (Index == 0) {
            return first();
        } else {
            return second();
        }
    }

    template <size_t Index>
    requires(Index <= 1)
    NODISCARD constexpr decltype(auto) get() && noexcept {
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
struct reversed_result<pair<First, Second, Pair>> {
    using type = pair<Second, First, Pair>;
};

template <typename Pair>
using reversed_result_t = typename reversed_result<Pair>::type;

/*! @cond TURN_OFF_DOXYGEN */
template <typename Ty>
concept _reversible_pair = _pair<Ty> && requires {
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
struct tuple_size<neutron::compressed_pair<First, Second>> :
    std::integral_constant<size_t, 2> {};

template <typename First, typename Second>
struct tuple_size<neutron::reversed_compressed_pair<First, Second>> :
    std::integral_constant<size_t, 2> {};

template <
    typename First, typename Second,
    template <typename, typename> typename Pair>
struct tuple_size<neutron::pair<First, Second, Pair>> :
    std::integral_constant<size_t, 2> {};

template <size_t Index, typename First, typename Second>
struct tuple_element<Index, neutron::compressed_pair<First, Second>> {
    static_assert(Index < 2);
    using pair = neutron::compressed_pair<First, Second>;
    using type = std::conditional_t<
        (Index == 0), typename pair::first_type, typename pair::second_type>;
};

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
struct tuple_element<Index, neutron::pair<First, Second, Pair>> {
    static_assert(Index < 2);
    using pair = neutron::pair<First, Second, Pair>;
    using type = std::conditional_t<
        (Index == 0), typename pair::first_type, typename pair::second_type>;
};

} // namespace std

/*! @endcond */
