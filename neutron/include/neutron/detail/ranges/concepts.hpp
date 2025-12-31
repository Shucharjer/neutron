#pragma once
#include <ranges>

namespace neutron {

// concept: ref_convertible
template <typename Rng, typename Container>
concept ref_convertible_range = !std::ranges::input_range<Container> ||
                                std::convertible_to<
                                    std::ranges::range_reference_t<Rng>,
                                    std::ranges::range_value_t<Container>>;

// concept: common_range
/**
 * @brief A range whose iterator type is the same as the sentinel type.
 *
 */
template <typename Rng>
concept common_range =
    ::std::ranges::range<Rng> &&
    ::std::same_as<
        std::ranges::iterator_t<Rng>, ::std::ranges::sentinel_t<Rng>>;

// concept: common_constructible
/**
 * @brief Containers that could be constructed form the given range and
 * arguments.
 *
 * Range should has input iterator, and we could construct the contains via
 * this.
 */
template <typename Rng, typename Container, typename... Args>
concept common_constructible_range =
    neutron::common_range<Rng> &&
    // using of iterator_category
    requires {
        typename std::iterator_traits<
            std::ranges::iterator_t<Rng>>::iterator_category;
    } &&
    // input_range_tag or derived from input_range_tag
    (std::same_as<
         typename std::iterator_traits<
             std::ranges::iterator_t<Rng>>::iterator_category,
         std::input_iterator_tag> ||
     std::derived_from<
         typename std::iterator_traits<
             std::ranges::iterator_t<Rng>>::iterator_category,
         std::input_iterator_tag>) &&
    // constructible from iterator (and arguments)
    std::constructible_from<
        Container, std::ranges::iterator_t<Rng>, std::ranges::iterator_t<Rng>,
        Args...>;

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

// concept: can_emplace_back
template <typename Container, typename Reference>
concept can_emplace_back =
    requires(Container& cnt) { cnt.emplace_back(std::declval<Reference>()); };

// concept: can_push_back
template <typename Container, typename Reference>
concept can_push_back =
    requires(Container& cnt) { cnt.push_back(std::declval<Reference>()); };

// concept: can_emplace_end
template <typename Container, typename Reference>
concept can_emplace_end = requires(Container& cnt) {
    cnt.emplace(cnt.end(), std::declval<Reference>());
};

// concept: can_insert_end
template <typename Container, typename Reference>
concept can_insert_end = requires(Container& cnt) {
    cnt.insert(cnt.end(), std::declval<Reference>());
};

} // namespace internal
/*! @endcond */

// concept: constructible_appendable
/**
 * @brief Contains could be constructed from arguments and then append elements
 * to it from range.
 *
 */
template <typename Rng, typename Container, typename... Args>
concept constructible_appendable_range =
    std::constructible_from<Container, Args...> &&
    (internal::can_emplace_back<
         Container, std::ranges::range_reference_t<Rng>> ||
     internal::can_push_back<Container, std::ranges::range_reference_t<Rng>> ||
     internal::can_emplace_end<
         Container, std::ranges::range_reference_t<Rng>> ||
     internal::can_insert_end<Container, std::ranges::range_reference_t<Rng>>);

template <typename Iter, typename Ty>
concept constructible_from_iterator =
    std::same_as<typename std::iterator_traits<Iter>::value_type, Ty>;

template <typename Rng, typename Ty>
concept compatible_range = std::ranges::range<Rng> &&
                           std::same_as<std::ranges::range_value_t<Rng>, Ty>;

} // namespace neutron
