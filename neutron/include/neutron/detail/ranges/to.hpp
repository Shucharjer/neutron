#pragma once
#include <ranges>

#if ATOM_HAS_CXX23 && false

namespace neutron::ranges {

using std::ranges::to;

}

#else

    #include "../iterator/phony_input_iterator.hpp"
    #include "./adaptor_closure.hpp"
    #include "./concepts.hpp"

namespace neutron::ranges {

// NOLINTBEGIN(cppcoreguidelines-require-return-statement)

/**
 * @brief Construct a container from a range and arguments.
 *
 */
template <typename Container, std::ranges::input_range Rng, typename... Args>
requires(!std::ranges::view<Container>)
[[nodiscard]] constexpr Container to(Rng&& range, Args&&... args) {
    static_assert(!std::is_const_v<Container>, "Container must not be const.");
    static_assert(
        !std::is_volatile_v<Container>, "Container must not be volatile.");
    static_assert(
        std::is_class_v<Container>, "Container must be a class type.");

    if constexpr (ref_convertible_range<Rng, Container>) {
        if constexpr (std::constructible_from<Container, Rng, Args...>) {
            return Container(
                std::forward<Rng>(range), std::forward<Args>(args)...);
        }
        // there is no std::from_range_t in cpp20
        else if constexpr (common_constructible_range<
                               Rng, Container, Args...>) {
            return Container(
                std::ranges::begin(range), std::ranges::end(range),
                std::forward<Args>(args)...);
        } else if constexpr (constructible_appendable_range<
                                 Rng, Container, Args...>) {
            Container cnt(std::forward<Args>(args)...);
            if constexpr (
                std::ranges::sized_range<Rng> &&
                std::ranges::sized_range<Container> &&
                requires(
                    Container& cnt,
                    const std::ranges::range_size_t<Container> count) {
                    cnt.reserve(count);
                    {
                        cnt.capacity()
                    } -> std::same_as<std::ranges::range_size_t<Container>>;
                    {
                        cnt.max_size()
                    } -> std::same_as<std::ranges::range_size_t<Container>>;
                }) {
                cnt.reserve(
                    static_cast<std::ranges::range_size_t<Container>>(
                        std::ranges::size(range)));
            }
            for (auto&& elem : range) {
                using element_type = decltype(elem);
                if constexpr (internal::can_emplace_back<
                                  Container, element_type>) {
                    cnt.emplace_back(std::forward<element_type>(elem));
                } else if constexpr (internal::can_push_back<
                                         Container, element_type>) {
                    cnt.push_back(std::forward<element_type>(elem));
                } else if constexpr (internal::can_emplace_end<
                                         Container, element_type>) {
                    cnt.emplace(cnt.end(), std::forward<element_type>(elem));
                } else {
                    cnt.insert(cnt.end(), std::forward<element_type>(elem));
                }
            }
            return cnt;
        } else {
            static_assert(
                false, "ranges::to requires the result to be constructible "
                       "from the source range, "
                       "either by using a suitable constructor, or by "
                       "inserting each element of the range "
                       "into the default-constructed object.");
        }
    } else if constexpr (std::ranges::input_range<
                             std::ranges::range_reference_t<Rng>>) {
        const auto form = []<typename Ty>(Ty&& elem) {
            return to<std::ranges::range_value_t<Container>>(
                std::forward<decltype(elem)>(elem));
        };
        return to<Container>(
            std::views::transform(std::ranges::ref_view{ range }, form),
            std::forward<Args>(args)...);
    } else {
        static_assert(
            false, "ranges::to requires the elements of the source range "
                   "to be either implicitly "
                   "convertible to the elements of the destination "
                   "container, or be ranges themselves "
                   "for ranges::to to be applied recursively.");
    }
}

// NOLINTEND(cppcoreguidelines-require-return-statement)

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template <typename Container>
struct to_class_fn {
    template <std::ranges::input_range Rng, typename... Args>
    [[nodiscard]] constexpr auto operator()(Rng&& range, Args&&... args)
    requires requires {
        to<Container>(std::forward<Rng>(range), std::forward<Args>(args)...);
    }
    {
        return to<Container>(
            std::forward<Rng>(range), std::forward<Args>(args)...);
    }
};

} // namespace internal
/*! @endcond */

/**
 * @brief Construct a container from a range.
 *
 */
template <typename Container, typename... Args>
[[nodiscard]] constexpr auto to(Args&&... args) {
    return make_closure<internal::to_class_fn<Container>>(
        std::forward<Args>(args)...);
}

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

// just for decltype
template <template <typename...> typename Cnt, typename Rng, typename... Args>
auto to_helper() {
    if constexpr (requires {
                      Cnt(std::declval<Rng>(), std::declval<Args>()...);
                  }) {
        return static_cast<decltype(Cnt(
            std::declval<Rng>(), std::declval<Args>()...))*>(nullptr);
    }
    // there is no std::from_range in cpp20
    /* else if constexpr (requires {
                           Cnt(std::from_range, std::declval<Rng>(),
    declval<Args>()...);
                       }) {
        return static_cast<
            decltype(Cnt(std::from_range, std::declval<Rng>(),
    std::declval<Args>()...))*>(nullptr);
    } */
    else if constexpr (requires {
                           Cnt(std::declval<phony_input_iterator<Rng>>(),
                               std::declval<phony_input_iterator<Rng>>(),
                               std::declval<Args>()...);
                       }) {
        return static_cast<decltype(Cnt(
            std::declval<phony_input_iterator<Rng>>(),
            std::declval<phony_input_iterator<Rng>>(),
            std::declval<Args>()...))*>(nullptr);
    } else {
        static_assert(
            false,
            "No suitable way to deduce the type of the container, please "
            "provide more information "
            "about the container to use other 'to' function template.");
    }
}

} // namespace internal
/*! @endcond */

/**
 * @brief Construct a container from a range.
 *
 */

/**
 * @brief Construct a container from a range.
 *
 * @tparam Container The container need to construct.
 * @tparam Rng The range.
 * @tparam Args Other arguments.
 * @param range
 * @param args
 * @return Deduced In most times, it could be deduced to as a completed type.
 */
template <
    template <typename...> typename Container, std::ranges::input_range Rng,
    typename... Args,
    typename Deduced = std::remove_pointer_t<
        decltype(internal::to_helper<Container, Rng, Args...>())>>
[[nodiscard]] constexpr auto to(Rng&& range, Args&&... args) -> Deduced {
    return to<Deduced>(std::forward<Rng>(range), std::forward<Args>(args)...);
}

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template <template <typename...> typename Container>
struct to_template_fn {
    template <
        std::ranges::input_range Rng, typename... Args,
        typename Deduced = std::remove_pointer_t<
            decltype(internal::to_helper<Container, Rng, Args...>())>>
    [[nodiscard]] constexpr auto operator()(Rng&& range, Args&&... args) {
        return to<Deduced>(
            std::forward<Rng>(range), std::forward<Args>(args)...);
    }
};

} // namespace internal
/*! @endcond */

// compiler and analyzer always deduce we are using another override.
// so, changed the function name
template <template <typename...> typename Container, typename... Args>
[[nodiscard]] constexpr auto to(Args&&... args) {
    return make_closure<internal::to_template_fn<Container>>(
        std::forward<Args>(args)...);
}

} // namespace neutron::ranges

#endif
