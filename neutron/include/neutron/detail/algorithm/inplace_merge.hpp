#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <numeric>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "../macros.hpp"

namespace neutron {

template <std::ranges::random_access_range... Ranges>
constexpr void
    _cycle_leader(Ranges&... ranges, std::vector<size_t>& indices, size_t size) noexcept(
        (std::is_nothrow_move_assignable_v<
             std::ranges::range_value_t<Ranges>> &&
         ...)) {
    constexpr auto is = std::index_sequence_for<Ranges...>();

    auto begins = std::forward_as_tuple(std::ranges::begin(ranges)...);

    for (size_t i = 0; i < size; ++i) {
        if (indices[i] == i) {
            continue;
        }

        auto curr = i;

        using tup = std::tuple<std::ranges::range_value_t<Ranges>...>;
        auto temp = std::apply(
            [curr](auto... begin) {
                return tup{ (*(begin + static_cast<ptrdiff_t>(curr)))... };
            },
            begins);

        while (true) {
            auto next     = indices[curr];
            indices[curr] = curr;

            if (next == i) {
                std::apply(
                    [curr, &temp](auto... begin) {
                        [&]<size_t... Is>(std::index_sequence<Is...>) {
                            ((*(begin + static_cast<ptrdiff_t>(curr)) =
                                  std::move(std::get<Is>(temp))),
                             ...);
                        }(is);
                    },
                    begins);
                break;
            }

            std::apply(
                [curr, next](auto... begin) {
                    ((*(begin + static_cast<ptrdiff_t>(curr)) =
                          std::move(*(begin + static_cast<ptrdiff_t>(next)))),
                     ...);
                },
                begins);
            curr = next;
        }
    }
}

template <
    std::ranges::random_access_range R,
    std::ranges::random_access_range... Ranges,
    typename Comp = std::ranges::less, typename Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj> &&
         (std::ranges::sized_range<Ranges> && ...) &&
         (std::permutable<std::ranges::iterator_t<Ranges>> && ...)
constexpr std::ranges::borrowed_iterator_t<R> inplace_merge(
    R& rng, std::ranges::iterator_t<R> middle, Ranges&... ranges,
    Comp comp = {}, Proj proj = {}) {
    const auto size = std::ranges::size(rng);
    if (size == 0) [[unlikely]] {
        return std::ranges::begin(rng);
    }

#if defined(DEBUG) || defined(_DEBUG)
    [[maybe_unused]] bool consistency =
        ((std::ranges::size(ranges) == size) && ...);
    assert(
        consistency &&
        "all ranges must have the same size with the main range.");
#endif

#if ATOM_HAS_CXX23
    thread_local std::vector<size_t> indices;
    if (indices.size() < size) [[unlikely]] {
        indices.resize(size);
    }
#else
    std::vector<size_t> indices(size);
#endif
    std::iota(indices.begin(), indices.begin() + size, 0);

    auto r_begin = std::ranges::begin(rng);

    auto dist = std::ranges::distance(r_begin, middle);

    auto compare = [r_begin, &comp, &proj](size_t lhs, size_t rhs) {
        return std::invoke(
            comp, std::invoke(proj, *(r_begin + static_cast<ptrdiff_t>(lhs))),
            std::invoke(proj, *(r_begin + static_cast<ptrdiff_t>(rhs))));
    };

    std::ranges::inplace_merge(
        indices.begin(), indices.begin() + dist, indices.end(), compare);

    _cycle_leader(rng, ranges..., indices, size);

    return std::ranges::begin(rng);
}

} // namespace neutron
