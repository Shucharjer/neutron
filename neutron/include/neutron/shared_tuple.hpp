#pragma once
#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace neutron {

/**
 * @class shared_tuple
 * @brief Tuple, but with pseudo-sharing considered.
 *
 * @tparam
 */
template <typename...>
class shared_tuple;

template <>
class shared_tuple<> {};

template <typename T, typename... Rest>
requires(sizeof...(Rest) != 0)
class shared_tuple<T, Rest...> {

    struct alignas(64) _aligned_t {
        T value;

        constexpr _aligned_t() noexcept(
            std::is_nothrow_default_constructible_v<T>) = default;

        template <typename Arg>
        requires std::constructible_from<T, Arg>
        constexpr _aligned_t(Arg&& arg) noexcept(
            std::is_nothrow_constructible_v<T, Arg>)
            : value(std::forward<Arg>(arg)) {}
    };

    _aligned_t current_;

    shared_tuple<Rest...> others_;

public:
    constexpr shared_tuple() noexcept(
        std::is_nothrow_default_constructible_v<_aligned_t> &&
        std::is_nothrow_default_constructible_v<shared_tuple<Rest...>>) =
        default;

    template <typename Arg, typename... Others>
    constexpr shared_tuple(Arg&& arg, Others&&... others) noexcept(
        std::is_nothrow_constructible_v<_aligned_t, Arg> &&
        std::is_nothrow_constructible_v<shared_tuple<Rest...>, Others...>)
        : current_(std::forward<Arg>(arg)),
          others_(std::forward<Others>(others)...) {}

    template <size_t Index>
    requires(Index <= sizeof...(Rest))
    constexpr auto& get() noexcept {
        if constexpr (Index == 0) {
            return current_.value;
        } else {
            return others_.template get<Index - 1>();
        }
    }

    template <size_t Index>
    requires(Index <= sizeof...(Rest))
    constexpr auto& get() const noexcept {
        if constexpr (Index == 0) {
            return current_.value;
        } else {
            return others_.template get<Index - 1>();
        }
    }
};

// an empty shared_tuple would cost 1 byte which uses an external line.
// so we need specifiy it.

template <typename T>
class alignas(64) shared_tuple<T> {
    T value_;

public:
    constexpr shared_tuple() noexcept(
        std::is_nothrow_default_constructible_v<T>) = default;

    template <typename Arg>
    requires std::constructible_from<T, Arg>
    constexpr shared_tuple(Arg&& arg) noexcept(
        std::is_nothrow_constructible_v<T, Arg>)
        : value_(std::forward<Arg>(arg)) {}

    template <size_t Index>
    requires(Index == 0)
    constexpr auto& get() noexcept {
        return value_;
    }

    template <size_t Index>
    requires(Index == 0)
    constexpr auto& get() const noexcept {
        return value_;
    }
};

} // namespace neutron

namespace std {

template <typename... Tys>
struct tuple_size<neutron::shared_tuple<Tys...>> {
    constexpr static size_t value = sizeof...(Tys);
};

template <size_t Index, typename... Tys>
struct tuple_element<Index, neutron::shared_tuple<Tys...>> {
    using type = std::tuple_element_t<Index, std::tuple<Tys...>>;
};

template <size_t Index, typename... Args>
constexpr auto& get(const neutron::shared_tuple<Args...>& tup) noexcept {
    return tup.template get<Index>();
}

template <size_t Index, typename... Args>
constexpr auto& get(neutron::shared_tuple<Args...>& tup) noexcept {
    return tup.template get<Index>();
}

} // namespace std
