#include <compare>
#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <neutron/utility.hpp>

using namespace neutron;

namespace {

struct empty {};
struct other_empty {};
struct final_empty final {};

/// Counts copies and moves to catch redundant element transfers.
struct counted {
    static inline int copies = 0;
    static inline int moves  = 0;
    static void reset() noexcept {
        copies = 0;
        moves  = 0;
    }

    int value = 0;

    counted() = default;
    counted(int val) noexcept : value(val) {}
    counted(const counted& that) noexcept : value(that.value) { ++copies; }
    counted(counted&& that) noexcept : value(that.value) {
        that.value = -1;
        ++moves;
    }
    counted& operator=(const counted& that) noexcept {
        value = that.value;
        ++copies;
        return *this;
    }
    counted& operator=(counted&& that) noexcept {
        value      = that.value;
        that.value = -1;
        ++moves;
        return *this;
    }
};

struct no_default {
    no_default(int) {}
};

struct explicit_only {
    explicit explicit_only(int) {}
};

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

using int_pair = compressed_pair<int, int>;

// Layout: empty elements take no space, other elements keep their layout.
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
static_assert(!std::is_trivially_copyable_v<compressed_pair<std::string, int>>);

// Constructors are constrained like `std::pair`.
static_assert(std::is_default_constructible_v<int_pair>);
static_assert(
    !std::is_default_constructible_v<compressed_pair<no_default, int>>);
static_assert(!std::is_constructible_v<int_pair, std::string, int>);
static_assert(!std::is_constructible_v<int_pair, int>);
static_assert(std::is_constructible_v<compressed_pair<int&, int>, int&, int>);
static_assert(!std::is_constructible_v<compressed_pair<int&, int>, int, int>);
static_assert(
    !std::is_copy_constructible_v<compressed_pair<std::unique_ptr<int>, int>>);
static_assert(std::is_nothrow_move_constructible_v<
              compressed_pair<std::unique_ptr<int>, int>>);

// `explicit` follows the elements.
static_assert(std::is_convertible_v<std::pair<int, int>, int_pair>);
static_assert(std::is_convertible_v<std::pair<short, long>, int_pair>);
static_assert(!std::is_convertible_v<
              std::pair<int, int>, compressed_pair<explicit_only, int>>);
static_assert(std::is_constructible_v<
              compressed_pair<explicit_only, int>, std::pair<int, int>>);
template <typename Pair>
concept copy_list_default = requires {
    [](Pair) {
    }({});
};
static_assert(copy_list_default<int_pair>);
static_assert(!copy_list_default<compressed_pair<explicit_default, int>>);

// Conversions from lvalues, const lvalues and rvalues of any pair.
static_assert(std::is_constructible_v<int_pair, std::pair<int, int>&>);
static_assert(std::is_constructible_v<int_pair, const std::pair<int, int>&>);
static_assert(
    std::is_constructible_v<int_pair, const compressed_pair<short, long>&>);
static_assert(std::is_constructible_v<int_pair, accessor_pair>);
static_assert(std::is_constructible_v<
              compressed_pair<std::unique_ptr<int>, int>,
              std::pair<std::unique_ptr<int>, int>&&>);
static_assert(!std::is_constructible_v<
              compressed_pair<std::unique_ptr<int>, int>,
              std::pair<std::unique_ptr<int>, int>&>);

// noexcept follows the source element types.
static_assert(std::is_nothrow_constructible_v<int_pair, std::pair<int, int>>);
static_assert(!std::is_nothrow_constructible_v<
              compressed_pair<std::string, int>, std::pair<const char*, int>>);
static_assert(
    std::is_nothrow_constructible_v<
        compressed_pair<std::string, int>, std::pair<std::string, int>&&>);

// Accessors propagate the value category.
using string_pair = compressed_pair<std::string, int>;
static_assert(std::is_same_v<
              decltype(std::declval<string_pair&>().first()), std::string&>);
static_assert(std::is_same_v<
              decltype(std::declval<const string_pair&>().first()),
              const std::string&>);
static_assert(std::is_same_v<
              decltype(std::declval<string_pair&&>().first()), std::string&&>);
static_assert(std::is_same_v<
              decltype(std::declval<const string_pair&&>().first()),
              const std::string&&>);
static_assert(std::is_same_v<
              decltype(std::declval<string_pair&&>().get<0>()), std::string&&>);
static_assert(
    std::is_same_v<
        decltype(std::declval<const string_pair&>().get<1>()), const int&>);
using pointer_pair = compressed_pair<int*, void (*)()>;
static_assert(
    std::is_same_v<decltype(std::declval<pointer_pair&&>().first()), int*&&>);
static_assert(std::is_same_v<
              decltype(std::declval<pointer_pair&&>().second()), void (*&&)()>);
using ref_pair = compressed_pair<int&, int>;
static_assert(
    std::is_same_v<decltype(std::declval<ref_pair&&>().first()), int&>);
static_assert(
    std::is_same_v<decltype(std::declval<const ref_pair&&>().first()), int&>);

static_assert(std::tuple_size_v<int_pair> == 2);
static_assert(
    std::is_same_v<std::tuple_element_t<0, string_pair>, std::string>);
static_assert(
    std::is_same_v<std::tuple_element_t<1, const string_pair>, const int>);

// Comparisons.
static_assert(std::equality_comparable<int_pair>);
static_assert(std::three_way_comparable<int_pair, std::strong_ordering>);
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
static_assert(mixed_comparable<int_pair, std::pair<int, int>>);
static_assert(mixed_comparable<int_pair, compressed_pair<long, short>>);
static_assert(
    std::is_same_v<decltype(int_pair{} <=> int_pair{}), std::strong_ordering>);
static_assert(
    std::is_same_v<
        decltype(compressed_pair<int, double>{} <=> std::pair<int, double>{}),
        std::partial_ordering>);
static_assert(
    std::is_same_v<
        decltype(compressed_pair<less_only, int>{} <=> compressed_pair<less_only, int>{}),
        std::weak_ordering>);
static_assert(!mixed_comparable<int_pair, std::pair<int, std::string>>);

constexpr bool test_constexpr() {
    int_pair value{};
    if (value.first() != 0 || value.second() != 0) {
        return false;
    }
    int_pair made{ 1, 2 };
    compressed_pair<long, long> converted = std::pair<int, int>{ 3, 4 };
    compressed_pair<int, long> piecewise(
        std::piecewise_construct, std::forward_as_tuple(5),
        std::forward_as_tuple(6));
    auto copy = made;
    copy      = std::pair<short, short>{ 7, 8 };
    copy.swap(made);
    const auto [first, second]     = made;
    const std::pair<int, int> back = copy;
    return first == 7 && second == 8 && back == std::pair<int, int>{ 1, 2 } &&
           converted.first() == 3 && piecewise.second() == 6 &&
           made == std::pair<int, int>{ 7, 8 } && (copy <=> made) < 0;
}
static_assert(test_constexpr());

bool test_conversions_move_once() {
    using pair = compressed_pair<counted, counted>;
    std::pair<counted, counted> source{ 1, 2 };
    counted::reset();

    pair from_lvalue{ source };
    if (counted::copies != 2 || counted::moves != 0) {
        return false;
    }
    counted::reset();
    pair from_rvalue{ std::move(source) };
    if (counted::copies != 0 || counted::moves != 2 ||
        from_rvalue.first().value != 1 || source.first.value != -1) {
        return false;
    }

    counted::reset();
    compressed_pair<counted, counted> from_other{ std::move(from_rvalue) };
    if (counted::moves != 2 || from_other.second().value != 2) {
        return false;
    }

    counted::reset();
    pair piecewise(
        std::piecewise_construct, std::forward_as_tuple(3),
        std::forward_as_tuple(from_other.first()));
    return counted::copies == 1 && counted::moves == 0 &&
           piecewise.first().value == 3 && piecewise.second().value == 1;
}

bool test_move_only() {
    using pair = compressed_pair<std::unique_ptr<int>, int>;
    pair value{ std::make_unique<int>(4), 5 };
    pair moved{ std::move(value) };
    if (value.first() != nullptr || *moved.first() != 4) {
        return false;
    }
    std::unique_ptr<int> taken = std::move(moved).first();
    std::pair<std::unique_ptr<int>, int> back =
        pair{ std::make_unique<int>(6), 7 };
    return *taken == 4 && moved.first() == nullptr && *back.first == 6;
}

bool test_pointers_and_references() {
    int target = 1;
    compressed_pair<int*, void (*)()> pointers{ &target, nullptr };
    auto moved = std::move(pointers);
    // Moving a pointer copies it, as with `std::pair`.
    if (pointers.first() != &target || moved.first() != &target) {
        return false;
    }

    compressed_pair<int&, int> refs{ target, 2 };
    refs.first()            = 3;
    std::move(refs).first() = 4;
    return target == 4;
}

bool test_assignment_and_swap() {
    compressed_pair<std::string, int> lhs{ "left", 1 };
    compressed_pair<std::string, int> rhs{ "right", 2 };
    swap(lhs, rhs);
    if (lhs.first() != "right" || rhs.second() != 1) {
        return false;
    }
    lhs = std::pair<const char*, short>{ "assigned", 3 };
    std::pair<std::string, int> source{ "moved", 4 };
    rhs = std::move(source);
    return lhs.first() == "assigned" && lhs.second() == 3 &&
           rhs.first() == "moved" && source.first.empty();
}

bool test_comparisons() {
    const int_pair value{ 1, 2 };
    const std::pair<long, long> same{ 1, 2 };
    const std::pair<int, int> greater{ 1, 3 };
    const accessor_pair accessor{ 1, 2 };
    // Both orders, so `==` and `!=` always agree.
    if (!(value == same) || !(same == value) || value != same ||
        same != value || !(value == accessor) || !(accessor == value)) {
        return false;
    }
    if (value == greater || !(value != greater) || !(value < greater) ||
        !(greater > value) || (value <=> greater) >= 0) {
        return false;
    }
    const compressed_pair<less_only, int> small{ less_only{ 1 }, 0 };
    const compressed_pair<less_only, int> big{ less_only{ 2 }, 0 };
    return small < big && !(big < small) && small == small;
}

bool test_empty_elements() {
    compressed_pair<empty, std::string> pair{ empty{}, "text" };
    auto& [nothing, text] = pair;
    static_cast<void>(nothing);
    return text == "text" && pair.second().size() == 4;
}

bool test_make_compressed_pair() {
    int target = 1;
    auto made  = make_compressed_pair(std::ref(target), 2.5);
    static_assert(
        std::is_same_v<decltype(made), compressed_pair<int&, double>>);
    made.first() = 7;
    auto decayed = make_compressed_pair("text", target);
    static_assert(
        std::is_same_v<decltype(decayed), compressed_pair<const char*, int>>);
    compressed_pair deduced{ 1, 2.0 };
    static_assert(
        std::is_same_v<decltype(deduced), compressed_pair<int, double>>);
    return target == 7 && decayed.second() == 7;
}

} // namespace

int main() {
    if (!test_conversions_move_once()) {
        return 1;
    }
    if (!test_move_only()) {
        return 2;
    }
    if (!test_pointers_and_references()) {
        return 3;
    }
    if (!test_assignment_and_swap()) {
        return 4;
    }
    if (!test_comparisons()) {
        return 5;
    }
    if (!test_empty_elements()) {
        return 6;
    }
    if (!test_make_compressed_pair()) {
        return 7;
    }
    return 0;
}
