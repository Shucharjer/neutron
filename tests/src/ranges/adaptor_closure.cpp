#include <algorithm>
#include <array>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>
#include <neutron/ranges.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Value category tags appended by the helper closures below.
enum category : int {
    lvalue       = 0,
    const_lvalue = 1,
    rvalue       = 2,
    const_rvalue = 3
};

using ints = std::vector<int>;

/// Copies the input and appends the value category of the closure itself.
struct tag_fn : range_adaptor_closure<tag_fn> {
    template <std::ranges::input_range Rng>
    ints operator()(Rng&& range) & {
        return append(range, lvalue);
    }
    template <std::ranges::input_range Rng>
    ints operator()(Rng&& range) const& {
        return append(range, const_lvalue);
    }
    template <std::ranges::input_range Rng>
    ints operator()(Rng&& range) && {
        return append(range, rvalue);
    }
    template <std::ranges::input_range Rng>
    ints operator()(Rng&& range) const&& {
        return append(range, const_rvalue);
    }

private:
    template <typename Rng>
    static ints append(Rng& range, int tag) {
        ints result(std::ranges::begin(range), std::ranges::end(range));
        result.push_back(tag);
        return result;
    }
};

/// Reports the value category of the range it receives.
struct range_category_fn : range_adaptor_closure<range_category_fn> {
    template <std::ranges::input_range Rng>
    ints operator()(Rng&&) const {
        return { std::is_lvalue_reference_v<Rng> ? lvalue : rvalue };
    }
};

/// Lazy adaptor: keeps the odd elements as a view.
struct odd_fn : range_adaptor_closure<odd_fn> {
    template <std::ranges::viewable_range Rng>
    constexpr auto operator()(Rng&& range) const {
        return std::forward<Rng>(range) | std::views::filter([](int value) {
                   return value % 2 != 0;
               });
    }
};

/// Stateful closure: adds its offset.
struct add_fn : range_adaptor_closure<add_fn> {
    int offset;

    constexpr explicit add_fn(int value) noexcept : offset(value) {}

    template <std::ranges::input_range Rng>
    constexpr ints operator()(Rng&& range) const {
        ints result;
        for (const int value : range) {
            result.push_back(value + offset);
        }
        return result;
    }
};

/// Move-only closure: multiplies by an owned factor.
struct scale_fn : range_adaptor_closure<scale_fn> {
    std::unique_ptr<int> factor;

    explicit scale_fn(int value) : factor(std::make_unique<int>(value)) {}

    template <std::ranges::input_range Rng>
    ints operator()(Rng&& range) const {
        ints result;
        for (const int value : range) {
            result.push_back(value * *factor);
        }
        return result;
    }
};

constexpr tag_fn tag{};
constexpr odd_fn odd{};

static_assert(std::is_empty_v<tag_fn>);
static_assert(std::is_empty_v<odd_fn>);
static_assert(std::derived_from<tag_fn, range_adaptor_closure<tag_fn>>);
static_assert(!std::is_copy_constructible_v<scale_fn>);

#if __cplusplus <= 202002L
// Implementation concepts of the C++20 fallback.
// satisfied by
static_assert(_range_adaptor_closure<tag_fn>);
static_assert(_range_adaptor_closure<const tag_fn&>);
static_assert(_range_adaptor_closure<scale_fn>);
static_assert(_range_adaptor_closure<decltype(tag | odd)>);
static_assert(_range_adaptor_closure_for<tag_fn, ints&>);
// rejected: non-closures and non-range arguments
static_assert(!_range_adaptor_closure<int>);
static_assert(!_range_adaptor_closure<ints>);
static_assert(!_range_adaptor_closure_for<tag_fn, int>);
#endif

// Closures work in constant expressions.
constexpr int sum_of_odds() {
    std::array<int, 4> array{ 1, 2, 3, 4 };
    int sum = 0;
    for (const int value : array | odd) {
        sum += value;
    }
    return sum;
}
static_assert(sum_of_odds() == 4);

constexpr int last_after_add_one() {
    std::array<int, 4> array{ 1, 2, 3, 4 };
    return (array | add_fn(1)).back();
}
static_assert(last_after_add_one() == 5);

void test_pipe_forwards_closure_category() {
    ints vec{ 7 };
    tag_fn closure;
    const tag_fn& const_closure = closure;

    require((vec | closure) == ints({ 7, lvalue }));
    require((vec | const_closure) == ints({ 7, const_lvalue }));
    require((vec | tag_fn{}) == ints({ 7, rvalue }));
    require((vec | std::move(const_closure)) == ints({ 7, const_rvalue }));
}

void test_pipe_forwards_range_category() {
    ints vec{ 7 };
    range_category_fn category;

    require((vec | category).front() == lvalue);
    require((ints{ 1 } | category).front() == rvalue);
    require((std::move(vec) | category).front() == rvalue);
}

void test_pipe_is_equivalent_to_call() {
    ints vec{ 1, 2, 3 };
    constexpr int offset = 3;

    require((vec | add_fn(offset)) == add_fn(offset)(vec));
}

void test_lazy_adaptor_yields_view() {
    ints vec{ 1, 2, 3, 4, 5 };

    auto odds = vec | odd;

    static_assert(std::ranges::view<decltype(odds)>);
    const ints expected{ 1, 3, 5 };
    require(std::ranges::equal(odds, expected));
}

void test_lazy_adaptor_mixes_with_std_adaptors() {
    ints vec{ 1, 2, 3, 4, 5 };

    auto reversed = vec | odd | std::views::reverse;
    auto taken    = vec | std::views::take(3) | odd;
    auto squares  = vec | odd | std::views::transform([](int value) {
                       return value * value;
                   });

    const ints expected_reversed{ 5, 3, 1 };
    const ints expected_taken{ 1, 3 };
    const ints expected_squares{ 1, 9, 25 };
    require(std::ranges::equal(reversed, expected_reversed));
    require(std::ranges::equal(taken, expected_taken));
    require(std::ranges::equal(squares, expected_squares));
}

void test_lazy_adaptor_owns_rvalue_range() {
    auto owned = ints{ 9, 8, 7 } | odd;

    const ints expected{ 9, 7 };
    require(std::ranges::equal(owned, expected));
}

void test_lazy_adaptor_on_empty_range() {
    ints empty;

    require(std::ranges::empty(empty | odd));
}

void test_composition_applies_in_order() {
    ints vec{ 1, 2, 3 };

    auto both = add_fn(1) | add_fn(10);

    const ints expected{ 12, 13, 14 };
    require((vec | both) == expected);
    require(both(vec) == expected);
}

void test_composition_is_composable() {
    ints vec{ 1, 2, 3 };
    auto both = add_fn(1) | add_fn(10);

    auto three   = both | add_fn(100);
    auto grouped = add_fn(1) | (add_fn(10) | add_fn(100));

    require((vec | three).front() == 112);
    // Composition is associative.
    require((vec | grouped) == (vec | three));
}

void test_composition_mixes_with_std_adaptors() {
    ints vec{ 1, 2, 3 };

    auto odd_reversed = odd | std::views::reverse;
    auto reversed_add = std::views::reverse | add_fn(1);
    auto pipeline     = std::views::drop(1) | odd |
                    std::views::transform([](int v) {
                        return v * 10;
                    }) |
                    add_fn(1);

    const ints expected_odd_reversed{ 3, 1 };
    require(std::ranges::equal(vec | odd_reversed, expected_odd_reversed));
    require((vec | reversed_add) == ints({ 4, 3, 2 }));
    require((vec | pipeline) == ints({ 31 }));
}

void test_composition_forwards_closure_category() {
    auto tags              = tag | tag;
    const auto& const_tags = tags;
    ints empty;

    require((empty | tags) == ints({ lvalue, lvalue }));
    require((empty | const_tags) == ints({ const_lvalue, const_lvalue }));
    require((empty | std::move(tags)) == ints({ rvalue, rvalue }));
    require((empty | (tag_fn{} | tag_fn{})) == ints({ rvalue, rvalue }));
}

void test_move_only_closure() {
    ints vec{ 1, 2, 3 };
    scale_fn triple(3);

    require((vec | scale_fn(2)) == ints({ 2, 4, 6 }));
    // Lvalue move-only closures are invoked directly.
    require(triple(vec) == ints({ 3, 6, 9 }));
}

void test_move_only_composition() {
    ints vec{ 1, 2, 3 };
    scale_fn triple(3);

    auto composed = scale_fn(2) | add_fn(1);

    const ints expected{ 3, 5, 7 };
    require(composed(vec) == expected);
    require(std::as_const(composed)(vec) == expected);
    require((vec | std::move(composed)) == expected);

    auto moved = std::move(triple) | scale_fn(2);
    require((vec | std::move(moved)) == ints({ 6, 12, 18 }));
}

} // namespace

int main() {
    test_pipe_forwards_closure_category();
    test_pipe_forwards_range_category();
    test_pipe_is_equivalent_to_call();
    test_lazy_adaptor_yields_view();
    test_lazy_adaptor_mixes_with_std_adaptors();
    test_lazy_adaptor_owns_rvalue_range();
    test_lazy_adaptor_on_empty_range();
    test_composition_applies_in_order();
    test_composition_is_composable();
    test_composition_mixes_with_std_adaptors();
    test_composition_forwards_closure_category();
    test_move_only_closure();
    test_move_only_composition();
    return neutron_test::result();
}
