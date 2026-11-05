#include <algorithm>
#include <list>
#include <memory>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <neutron/ranges.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

enum class category : unsigned char {
    lvalue,
    const_lvalue,
    rvalue,
    const_rvalue
};

template <typename Ty>
constexpr category category_of() noexcept {
    using raw = std::remove_reference_t<Ty>;
    if constexpr (std::is_lvalue_reference_v<Ty>) {
        return std::is_const_v<raw> ? category::const_lvalue : category::lvalue;
    } else {
        return std::is_const_v<raw> ? category::const_rvalue : category::rvalue;
    }
}

/// Reports how the call argument and the stored argument arrive.
struct inspect_fn {
    template <typename Arg, typename Stored>
    constexpr std::pair<category, category>
        operator()(Arg&&, Stored&&) const noexcept {
        return { category_of<Arg&&>(), category_of<Stored&&>() };
    }
};

/// Adds a stored offset to each element of an input range.
struct add_fn {
    template <std::ranges::input_range Rng>
    constexpr std::vector<int> operator()(Rng&& range, int offset) const {
        std::vector<int> result;
        for (const int value : range) {
            result.push_back(value + offset);
        }
        return result;
    }
};

/// Builds a vector from an iterator pair, the end iterator being stored.
struct get_vector_fn {
    template <typename It>
    requires std::input_iterator<It>
    std::vector<int> operator()(It first, It last) const {
        return std::vector<int>(first, last);
    }
};

/// Increments a stored counter, so state must persist in lvalue closures.
struct count_fn {
    constexpr int operator()(int step, int& counter) const noexcept {
        counter += step;
        return counter;
    }
};

/// Accepts a move-only call argument.
struct deref_fn {
    int operator()(std::unique_ptr<int> ptr, int offset) const {
        return *ptr + offset;
    }
};

/// Reads a move-only stored argument.
struct read_ptr_fn {
    int operator()(int offset, const std::unique_ptr<int>& ptr) const {
        return *ptr + offset;
    }
};

/// Potentially-throwing callable.
struct throwing_fn {
    int operator()(int value) const { return value; }
};

/// Doubles every element; a user-defined range adaptor closure.
struct twice_fn : range_adaptor_closure<twice_fn> {
    template <std::ranges::input_range Rng>
    constexpr std::vector<int> operator()(Rng&& range) const {
        std::vector<int> result;
        for (const int value : range) {
            result.push_back(value * 2);
        }
        return result;
    }
};

constexpr twice_fn twice{};

// make_closure decays its arguments.
static_assert(std::same_as<
              decltype(make_closure<inspect_fn>(std::declval<std::string&>())),
              closure<inspect_fn, std::string>>);
static_assert(std::same_as<
              decltype(make_closure<inspect_fn>("literal")),
              closure<inspect_fn, const char*>>);
static_assert(std::same_as<
              decltype(make_closure<add_fn>(std::declval<const int&>())),
              closure<add_fn, int>>);
static_assert(
    std::same_as<decltype(make_closure<throwing_fn>()), closure<throwing_fn>>);

// A closure is a range adaptor closure.
static_assert(
    std::derived_from<
        closure<add_fn, int>, range_adaptor_closure<closure<add_fn, int>>>);

// The constructor is explicit and constrained.
static_assert(std::is_constructible_v<closure<add_fn, int>, int>);
static_assert(!std::is_convertible_v<int, closure<add_fn, int>>);
static_assert(!std::is_constructible_v<closure<add_fn, int>, std::string>);
static_assert(std::is_nothrow_constructible_v<closure<add_fn, int>, int>);
static_assert(!std::is_nothrow_constructible_v<
              closure<inspect_fn, std::string>, const char*>);

// Invocability is constrained on the function object.
static_assert(std::is_invocable_v<closure<add_fn, int>&, std::vector<int>&>);
static_assert(!std::is_invocable_v<closure<add_fn, int>&, int>);
static_assert(!std::is_invocable_v<closure<count_fn, int>&, std::string>);
// count_fn needs a mutable stored argument.
static_assert(std::is_invocable_v<closure<count_fn, int>&, int>);
static_assert(!std::is_invocable_v<const closure<count_fn, int>&, int>);

// noexcept follows the function object.
static_assert(std::is_nothrow_invocable_v<closure<count_fn, int>&, int>);
static_assert(
    std::is_nothrow_invocable_v<const closure<inspect_fn, int>&, int>);
static_assert(!std::is_nothrow_invocable_v<closure<throwing_fn>&, int>);

// Move-only stored arguments make a move-only closure.
static_assert(
    !std::is_copy_constructible_v<closure<read_ptr_fn, std::unique_ptr<int>>>);
static_assert(
    std::is_move_constructible_v<closure<read_ptr_fn, std::unique_ptr<int>>>);

// State persists across calls in constant evaluation.
constexpr int count_twice() {
    auto counter = make_closure<count_fn>(0);
    counter(2);
    return counter(3);
}
static_assert(count_twice() == 5);

void test_call_argument_is_forwarded() {
    auto inspect     = make_closure<inspect_fn>(std::string("stored"));
    int value        = 0;
    const int cvalue = 0;

    require(inspect(value).first == category::lvalue);
    require(inspect(cvalue).first == category::const_lvalue);
    require(inspect(0).first == category::rvalue);
    require(inspect(std::move(cvalue)).first == category::const_rvalue);
}

void test_stored_argument_follows_closure_constness() {
    auto inspect              = make_closure<inspect_fn>(std::string("stored"));
    const auto& const_inspect = inspect;

    require(inspect(0).second == category::lvalue);
    require(const_inspect(0).second == category::const_lvalue);
}

void test_rvalue_closure_is_invocable() {
    auto inspect              = make_closure<inspect_fn>(std::string("stored"));
    const auto& const_inspect = inspect;
    int value                 = 0;
    auto copy                 = inspect;

    require(std::move(copy)(value).first == category::lvalue);
    require(std::move(const_inspect)(0).first == category::rvalue);
}

void test_lvalue_closure_keeps_state() {
    constexpr int initial = 10;
    auto counter          = make_closure<count_fn>(initial);

    require(counter(1) == 11);
    require(counter(2) == 13);
}

void test_copies_carry_their_own_state() {
    constexpr int initial = 10;
    auto counter          = make_closure<count_fn>(initial);
    counter(1);
    counter(2);

    // The copy starts from the current state, then both evolve separately.
    auto copy = counter;

    require(copy(1) == 14);
    require(counter(1) == 14);
}

void test_stored_end_iterator() {
    const std::vector<int> origin{ 1, 2, 3, 4, 5, 6 };
    auto from_here = make_closure<get_vector_fn>(origin.cend());

    auto tail = from_here(origin.cbegin() + 2);

    const std::vector<int> expected{ 3, 4, 5, 6 };
    require(tail == expected);
    require(from_here(origin.cend()).empty());
}

void test_move_only_call_argument() {
    auto deref = make_closure<deref_fn>(1);

    require(deref(std::make_unique<int>(41)) == 42);
}

void test_move_only_stored_argument() {
    auto read = make_closure<read_ptr_fn>(std::make_unique<int>(7));
    require(read(1) == 8);

    auto moved         = std::move(read);
    const auto& cmoved = moved;

    require(moved(2) == 9);
    require(cmoved(3) == 10);
}

void test_range_pipe_closure() {
    std::vector<int> vec{ 1, 2, 3 };
    auto add_ten     = make_closure<add_fn>(10);
    const auto& cadd = add_ten;

    require((vec | add_ten) == std::vector<int>({ 11, 12, 13 }));
    require((vec | make_closure<add_fn>(1)) == std::vector<int>({ 2, 3, 4 }));
    require((vec | cadd).front() == 11);
    require((vec | std::move(add_ten)).back() == 13);
}

void test_pipe_accepts_rvalue_ranges_and_views() {
    std::vector<int> vec{ 1, 2, 3 };
    std::list<int> list{ 4, 5 };

    require((std::vector<int>{ 5 } | make_closure<add_fn>(1)).front() == 6);
    require((vec | std::views::reverse | make_closure<add_fn>(0)).front() == 3);
    require((list | make_closure<add_fn>(1)).back() == 6);
}

void test_closure_pipe_closure() {
    std::vector<int> vec{ 1, 2, 3 };
    auto add_one = make_closure<add_fn>(1);

    auto add_then_x2 = add_one | twice;
    auto x2_then_add = twice | add_one;
    auto chain       = add_one | make_closure<add_fn>(2) | twice;

    require((vec | add_then_x2) == std::vector<int>({ 4, 6, 8 }));
    require(add_then_x2(vec).front() == 4);
    require((vec | x2_then_add).front() == 3);
    require((vec | chain).front() == 8);

    std::vector<int> empty;
    require((empty | chain).empty());
}

void test_closure_mixes_with_std_adaptors() {
    std::vector<int> vec{ 1, 2, 3 };
    auto add_one = make_closure<add_fn>(1);

    auto rev_add  = std::views::reverse | add_one;
    auto add_rev  = add_one | std::views::reverse;
    auto mixed    = std::views::take(2) | add_one | std::views::reverse | twice;
    auto filtered = std::views::filter([](int value) {
                        return value != 2;
                    }) |
                    add_one;

    require((vec | rev_add).front() == 4);
    require(*(vec | add_rev).begin() == 4);
    require((vec | mixed) == std::vector<int>({ 6, 4 }));
    require((vec | filtered) == std::vector<int>({ 2, 4 }));
}

} // namespace

int main() {
    test_call_argument_is_forwarded();
    test_stored_argument_follows_closure_constness();
    test_rvalue_closure_is_invocable();
    test_lvalue_closure_keeps_state();
    test_copies_carry_their_own_state();
    test_stored_end_iterator();
    test_move_only_call_argument();
    test_move_only_stored_argument();
    test_range_pipe_closure();
    test_pipe_accepts_rvalue_ranges_and_views();
    test_closure_pipe_closure();
    test_closure_mixes_with_std_adaptors();
    return neutron_test::result();
}
