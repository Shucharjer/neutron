// Collect ranges into containers and write custom pipeable adaptors.
#include <ranges>
#include <utility>
#include <vector>
#include <neutron/print.hpp>
#include <neutron/ranges.hpp>

/// Multiplies every element of a range by a factor.
struct scale_fn {
    template <std::ranges::viewable_range Rng>
    constexpr auto operator()(Rng&& range, int factor) const {
        return std::forward<Rng>(range) |
               std::views::transform([factor](int value) {
                   return value * factor;
               });
    }
};

/// `range | scale(n)`; the factor is stored in the closure.
constexpr auto scale(int factor) {
    return neutron::make_closure<scale_fn>(factor);
}

int main() {
    const std::vector numbers{ 1, 2, 3, 4, 5 };
    auto is_odd = [](int value) {
        return value % 2 != 0;
    };

    auto result = numbers | std::views::filter(is_odd) | scale(10) |
                  neutron::ranges::to<std::vector>();

    for (const int value : result) {
        neutron::print("{} ", value);
    }
    neutron::println("");
}
