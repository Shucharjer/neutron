// IWYU pragma: private, include <neutron/algorithm.hpp>
#pragma once
#include <bit>
#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>

namespace neutron {

template <
    std::random_access_iterator RandomAccessIter, typename Val, typename Comp>
requires std::strict_weak_order<Comp, std::iter_value_t<RandomAccessIter>, Val>
constexpr auto branchless_lower_bound(
    RandomAccessIter first, RandomAccessIter last, const Val& val,
    Comp&& comp) noexcept {
    std::size_t length = last - first;
    if (length == 0) {
        return last;
    }

    std::size_t step = std::bit_floor(length);
    if (step != length && comp(first[step], val)) {
        length -= step + 1;
        if (length == 0) {
            return last;
        }
        step  = std::bit_ceil(length);
        first = last - step;
    }
    for (step /= 2; step != 0; step /= 2) {
        if (comp(first[step], val)) {
            first += step;
        }
    }
    return first + comp(*first, val);
}

template <std::random_access_iterator RandomAccessIter, typename Val>
constexpr auto branchless_lower_bound(
    RandomAccessIter first, RandomAccessIter last, const Val& val) noexcept {
    return branchless_lower_bound(first, last, val, std::less<>{});
}

namespace ranges {

struct _branchless_lower_bound_t {
    template <std::ranges::random_access_range Rng, typename Comp = std::less<>>
    constexpr auto operator()(const Rng& range, Comp&& comp = {}) const {
        using namespace std::ranges;
        return ::neutron::branchless_lower_bound(
            begin(range), end(range), std::forward<Comp>(comp));
    }
};

inline constexpr _branchless_lower_bound_t branchless_lower_bound;

} // namespace ranges

} // namespace neutron
