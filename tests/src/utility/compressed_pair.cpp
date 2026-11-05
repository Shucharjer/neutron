#include <compare>
#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Empty types eligible for the empty-base optimisation.
struct empty {};
struct other_empty {};

/// Empty type that cannot be used as a base class.
struct final_empty final {};

/// Counts copies and moves to catch redundant element transfers.
struct counted {
    static inline int copies = 0;
    static inline int moves  = 0;
    static void reset() noexcept {
        copies = 0;
        moves  = 0;
    }

    /// Value left behind by a move.
    static constexpr int moved_from = -1;

    int value = 0;

    counted() = default;
    counted(int val) noexcept : value(val) {}
    counted(const counted& that) noexcept : value(that.value) { ++copies; }
    counted(counted&& that) noexcept : value(that.value) {
        that.value = moved_from;
        ++moves;
    }
    counted& operator=(const counted& that) noexcept {
        value = that.value;
        ++copies;
        return *this;
    }
    counted& operator=(counted&& that) noexcept {
        value      = that.value;
        that.value = moved_from;
        ++moves;
        return *this;
    }
};

/// Not default constructible.
struct no_default {
    no_default(int) {}
};

/// Constructible from int only explicitly.
struct explicit_only {
    explicit explicit_only(int) {}
};

/// Default constructible only explicitly.
struct explicit_default {
    explicit explicit_default() = default;
};

/// Pair-like type exposing accessors, like `compressed_pair` itself.
struct accessor_pair {
    using first_type  = int;
    using second_type = long;
    int a;
    long b;
    constexpr int first() const noexcept { return a; }
    constexpr long second() const noexcept { return b; }
};

/// Only `<` is available, as with many legacy types.
struct less_only {
    int value;
    friend bool operator<(const less_only& lhs, const less_only& rhs) {
        return lhs.value < rhs.value;
    }
    friend bool operator==(const less_only&, const less_only&) = default;
};

/// Satisfied when `Pair` can be copy-list-initialised from `{}`.
template <typename Pair>
concept copy_list_default = requires {
    [](Pair) {
    }({});
};

/// Satisfied when both operand orders support every comparison operator.
template <typename Lhs, typename Rhs>
concept mixed_comparable = requires(const Lhs& lhs, const Rhs& rhs) {
    { lhs == rhs } -> std::same_as<bool>;
    { rhs == lhs } -> std::same_as<bool>;
    { lhs != rhs } -> std::same_as<bool>;
    { rhs != lhs } -> std::same_as<bool>;
    lhs <=> rhs;
    rhs <=> lhs;
    lhs < rhs;
    rhs < lhs;
};

using int_pair     = compressed_pair<int, int>;
using string_pair  = compressed_pair<std::string, int>;
using pointer_pair = compressed_pair<int*, void (*)()>;
using ref_pair     = compressed_pair<int&, int>;
using unique_pair  = compressed_pair<std::unique_ptr<int>, int>;

/// Result type of `first()` called on a pair of type `Self`.
template <typename Self>
using first_result = decltype(std::declval<Self>().first());

/// Result type of `second()` called on a pair of type `Self`.
template <typename Self>
using second_result = decltype(std::declval<Self>().second());

// Empty elements take no space; other elements keep their layout.
static_assert(sizeof(compressed_pair<empty, int>) == sizeof(int));
static_assert(sizeof(compressed_pair<int, empty>) == sizeof(int));
static_assert(sizeof(compressed_pair<final_empty, int>) == sizeof(int));
static_assert(sizeof(compressed_pair<empty, other_empty>) == 1);
static_assert(std::is_empty_v<compressed_pair<empty, other_empty>>);
static_assert(
    sizeof(compressed_pair<int, char>) == sizeof(std::pair<int, char>));
static_assert(
    sizeof(compressed_pair<std::pair<int, char>, char>) ==
    sizeof(std::pair<std::pair<int, char>, char>));
static_assert(alignof(compressed_pair<char, double>) == alignof(double));

// Triviality follows the elements.
static_assert(std::is_trivially_copyable_v<int_pair>);
static_assert(std::is_trivially_copyable_v<compressed_pair<int*, empty>>);
static_assert(std::is_trivially_copyable_v<compressed_pair<void (*)(), int>>);
static_assert(std::is_trivially_destructible_v<int_pair>);
static_assert(!std::is_trivially_copyable_v<string_pair>);

// Constructors are constrained like `std::pair`.
static_assert(std::is_default_constructible_v<int_pair>);
static_assert(
    !std::is_default_constructible_v<compressed_pair<no_default, int>>);
static_assert(!std::is_constructible_v<int_pair, std::string, int>);
static_assert(!std::is_constructible_v<int_pair, int>);
static_assert(std::is_constructible_v<ref_pair, int&, int>);
static_assert(!std::is_constructible_v<ref_pair, int, int>);
static_assert(!std::is_copy_constructible_v<unique_pair>);
static_assert(std::is_nothrow_move_constructible_v<unique_pair>);

// `explicit` follows the elements.
static_assert(std::is_convertible_v<std::pair<int, int>, int_pair>);
static_assert(std::is_convertible_v<std::pair<short, long>, int_pair>);
static_assert(!std::is_convertible_v<
              std::pair<int, int>, compressed_pair<explicit_only, int>>);
static_assert(std::is_constructible_v<
              compressed_pair<explicit_only, int>, std::pair<int, int>>);
static_assert(copy_list_default<int_pair>);
static_assert(!copy_list_default<compressed_pair<explicit_default, int>>);

// Conversions from lvalues, const lvalues and rvalues of any pair.
static_assert(std::is_constructible_v<int_pair, std::pair<int, int>&>);
static_assert(std::is_constructible_v<int_pair, const std::pair<int, int>&>);
static_assert(
    std::is_constructible_v<int_pair, const compressed_pair<short, long>&>);
static_assert(std::is_constructible_v<int_pair, accessor_pair>);
static_assert(std::is_constructible_v<
              unique_pair, std::pair<std::unique_ptr<int>, int>&&>);
static_assert(!std::is_constructible_v<
              unique_pair, std::pair<std::unique_ptr<int>, int>&>);

// noexcept follows the source element types.
static_assert(std::is_nothrow_constructible_v<int_pair, std::pair<int, int>>);
static_assert(
    !std::is_nothrow_constructible_v<string_pair, std::pair<const char*, int>>);
static_assert(std::is_nothrow_constructible_v<
              string_pair, std::pair<std::string, int>&&>);

// Accessors propagate the value category.
static_assert(std::is_same_v<first_result<string_pair&>, std::string&>);
static_assert(
    std::is_same_v<first_result<const string_pair&>, const std::string&>);
static_assert(std::is_same_v<first_result<string_pair&&>, std::string&&>);
static_assert(
    std::is_same_v<first_result<const string_pair&&>, const std::string&&>);
static_assert(std::is_same_v<
              decltype(std::declval<string_pair&&>().get<0>()), std::string&&>);
static_assert(
    std::is_same_v<
        decltype(std::declval<const string_pair&>().get<1>()), const int&>);

// Pointer elements are returned like any other value.
static_assert(std::is_same_v<first_result<pointer_pair&&>, int*&&>);
static_assert(std::is_same_v<second_result<pointer_pair&&>, void (*&&)()>);

// Reference elements collapse to lvalue references.
static_assert(std::is_same_v<first_result<ref_pair&&>, int&>);
static_assert(std::is_same_v<first_result<const ref_pair&&>, int&>);

// Tuple protocol
static_assert(std::tuple_size_v<int_pair> == 2);
static_assert(
    std::is_same_v<std::tuple_element_t<0, string_pair>, std::string>);
static_assert(
    std::is_same_v<std::tuple_element_t<1, const string_pair>, const int>);

// Comparisons work with any pair-like type of comparable elements.
static_assert(std::equality_comparable<int_pair>);
static_assert(std::three_way_comparable<int_pair, std::strong_ordering>);
static_assert(mixed_comparable<int_pair, std::pair<int, int>>);
static_assert(mixed_comparable<int_pair, compressed_pair<long, short>>);
static_assert(!mixed_comparable<int_pair, std::pair<int, std::string>>);

// The comparison category is the common one of the elements.
static_assert(
    std::is_same_v<decltype(int_pair{} <=> int_pair{}), std::strong_ordering>);
static_assert(
    std::is_same_v<
        decltype(compressed_pair<int, double>{} <=> std::pair<int, double>{}),
        std::partial_ordering>);
// `<` alone is synthesised into a weak ordering.
static_assert(
    std::is_same_v<
        decltype(compressed_pair<less_only, int>{} <=> compressed_pair<less_only, int>{}),
        std::weak_ordering>);

constexpr bool default_construction_zeroes_in_constexpr() {
    int_pair value{};
    return value.first() == 0 && value.second() == 0;
}
static_assert(default_construction_zeroes_in_constexpr());

constexpr bool conversion_from_std_pair_in_constexpr() {
    compressed_pair<long, long> converted = std::pair<int, int>{ 3, 4 };
    return converted.first() == 3;
}
static_assert(conversion_from_std_pair_in_constexpr());

constexpr bool piecewise_construction_in_constexpr() {
    compressed_pair<int, long> piecewise(
        std::piecewise_construct, std::forward_as_tuple(5),
        std::forward_as_tuple(6));
    return piecewise.second() == 6;
}
static_assert(piecewise_construction_in_constexpr());

constexpr bool assign_and_swap_in_constexpr() {
    int_pair made{ 1, 2 };
    auto copy = made;

    copy = std::pair<short, short>{ 7, 8 };
    copy.swap(made);

    const auto [first, second]     = made;
    const std::pair<int, int> back = copy;
    return first == 7 && second == 8 && back == std::pair<int, int>{ 1, 2 } &&
           made == std::pair<int, int>{ 7, 8 } && (copy <=> made) < 0;
}
static_assert(assign_and_swap_in_constexpr());

void test_conversion_from_lvalue_pair_copies_each_element_once() {
    std::pair<counted, counted> source{ 1, 2 };
    counted::reset();

    compressed_pair<counted, counted> from_lvalue{ source };

    require(counted::copies == 2);
    require(counted::moves == 0);
}

void test_conversion_from_rvalue_pair_moves_each_element_once() {
    std::pair<counted, counted> source{ 1, 2 };
    counted::reset();

    compressed_pair<counted, counted> from_rvalue{ std::move(source) };

    require(counted::copies == 0);
    require(counted::moves == 2);
    require(from_rvalue.first().value == 1);
    require(source.first.value == counted::moved_from);
}

void test_move_construction_moves_each_element_once() {
    compressed_pair<counted, counted> source{ 1, 2 };
    counted::reset();

    compressed_pair<counted, counted> moved{ std::move(source) };

    require(counted::moves == 2);
    require(moved.second().value == 2);
}

void test_piecewise_construction_forwards_arguments() {
    counted existing{ 1 };
    counted::reset();

    compressed_pair<counted, counted> piecewise(
        std::piecewise_construct, std::forward_as_tuple(3),
        std::forward_as_tuple(existing));

    require(counted::copies == 1);
    require(counted::moves == 0);
    require(piecewise.first().value == 3);
    require(piecewise.second().value == 1);
}

void test_move_only_element_is_moved() {
    unique_pair value{ std::make_unique<int>(4), 5 };

    unique_pair moved{ std::move(value) };

    require(value.first() == nullptr);
    require_or_return(moved.first() != nullptr, );
    require(*moved.first() == 4);
}

void test_rvalue_accessor_moves_out_the_element() {
    unique_pair value{ std::make_unique<int>(4), 5 };

    std::unique_ptr<int> taken = std::move(value).first();

    require(value.first() == nullptr);
    require_or_return(taken != nullptr, );
    require(*taken == 4);
}

void test_move_only_pair_converts_to_std_pair() {
    std::pair<std::unique_ptr<int>, int> back =
        unique_pair{ std::make_unique<int>(6), 7 };

    require_or_return(back.first != nullptr, );
    require(*back.first == 6);
}

void test_moving_pointers_copies_them() {
    int target = 1;
    pointer_pair pointers{ &target, nullptr };

    auto moved = std::move(pointers);

    // Same as `std::pair`: the source keeps its pointer.
    require(pointers.first() == &target);
    require(moved.first() == &target);
}

void test_reference_element_writes_through() {
    int target = 1;
    ref_pair refs{ target, 2 };

    refs.first() = 3;
    require(target == 3);

    std::move(refs).first() = 4;
    require(target == 4);
}

void test_free_swap_exchanges_elements() {
    string_pair lhs{ "left", 1 };
    string_pair rhs{ "right", 2 };

    swap(lhs, rhs);

    require(lhs.first() == "right");
    require(rhs.second() == 1);
}

void test_assignment_from_converting_pair() {
    string_pair target{ "left", 1 };

    target = std::pair<const char*, short>{ "assigned", 3 };

    require(target.first() == "assigned");
    require(target.second() == 3);
}

void test_assignment_from_rvalue_pair_moves() {
    string_pair target{ "right", 2 };
    std::pair<std::string, int> source{ "moved", 4 };

    target = std::move(source);

    require(target.first() == "moved");
    require(source.first.empty());
}

void test_equality_with_other_pair_likes() {
    const int_pair value{ 1, 2 };
    const std::pair<long, long> same{ 1, 2 };
    const accessor_pair accessor{ 1, 2 };

    // Both operand orders, so `==` and `!=` always agree.
    require(value == same);
    require(same == value);
    require_false(value != same);
    require_false(same != value);
    require(value == accessor);
    require(accessor == value);
}

void test_ordering_with_other_pair_likes() {
    const int_pair value{ 1, 2 };
    const std::pair<int, int> greater{ 1, 3 };

    require_false(value == greater);
    require(value != greater);
    require(value < greater);
    require(greater > value);
    require((value <=> greater) < 0);
}

void test_ordering_with_less_only_elements() {
    const compressed_pair<less_only, int> small{ less_only{ 1 }, 0 };
    const compressed_pair<less_only, int> big{ less_only{ 2 }, 0 };

    require(small < big);
    require_false(big < small);
    require(small == small);
}

void test_structured_binding_with_empty_element() {
    compressed_pair<empty, std::string> pair{ empty{}, "text" };

    auto& [nothing, text] = pair;
    static_cast<void>(nothing);

    require(text == "text");
    require(pair.second().size() == 4);
}

void test_make_compressed_pair_unwraps_reference_wrapper() {
    int target = 1;

    auto made = make_compressed_pair(std::ref(target), 2.5);
    static_assert(
        std::is_same_v<decltype(made), compressed_pair<int&, double>>);
    made.first() = 7;

    require(target == 7);
}

void test_make_compressed_pair_decays_arguments() {
    int target = 7;

    auto decayed = make_compressed_pair("text", target);
    static_assert(
        std::is_same_v<decltype(decayed), compressed_pair<const char*, int>>);

    require(decayed.second() == 7);
}

void test_deduction_guide_decays_arguments() {
    compressed_pair deduced{ 1, 2.0 };
    static_assert(
        std::is_same_v<decltype(deduced), compressed_pair<int, double>>);
}

} // namespace

int main() {
    test_conversion_from_lvalue_pair_copies_each_element_once();
    test_conversion_from_rvalue_pair_moves_each_element_once();
    test_move_construction_moves_each_element_once();
    test_piecewise_construction_forwards_arguments();
    test_move_only_element_is_moved();
    test_rvalue_accessor_moves_out_the_element();
    test_move_only_pair_converts_to_std_pair();
    test_moving_pointers_copies_them();
    test_reference_element_writes_through();
    test_free_swap_exchanges_elements();
    test_assignment_from_converting_pair();
    test_assignment_from_rvalue_pair_moves();
    test_equality_with_other_pair_likes();
    test_ordering_with_other_pair_likes();
    test_ordering_with_less_only_elements();
    test_structured_binding_with_empty_element();
    test_make_compressed_pair_unwraps_reference_wrapper();
    test_make_compressed_pair_decays_arguments();
    test_deduction_guide_decays_arguments();
    return neutron_test::result();
}
