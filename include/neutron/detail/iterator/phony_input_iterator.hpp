// IWYU pragma: private, include <neutron/iterator.hpp>
#pragma once
#include <ranges>

namespace neutron {

/**
 * @brief Phony input iterator, for lazy construction with ranges.
 *
 * Can not really iterate.
 * @tparam Rng
 */
template <std::ranges::input_range Rng>
struct phony_input_iterator {
    using iterator_category = std::input_iterator_tag;
    using value_type        = std::ranges::range_value_t<Rng>;
    using difference_type   = ptrdiff_t;
    using pointer   = std::add_pointer_t<std::ranges::range_reference_t<Rng>>;
    using reference = std::ranges::range_reference_t<Rng>;

    reference operator*() const = delete;
    pointer operator->() const  = delete;

    phony_input_iterator& operator++()   = delete;
    phony_input_iterator operator++(int) = delete;

    bool operator==(const phony_input_iterator&) const = delete;
    bool operator!=(const phony_input_iterator&) const = delete;
};

} // namespace neutron
