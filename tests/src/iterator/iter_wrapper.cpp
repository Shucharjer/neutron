#include <algorithm>
#include <array>
#include <compare>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <neutron/iterator.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Mutable wrapper, the `iterator` of a contiguous container.
using iter = _iter_wrapper<int*>;
/// Read-only wrapper, the matching `const_iterator`.
using const_iter = _iter_wrapper<const int*>;
/// Wrapper over a class type, for `operator->` and iter_move.
using string_iter = _iter_wrapper<std::string*>;
/// Subrange of wrappers, for range concepts and algorithms.
using subrange = std::ranges::subrange<iter>;

// Member types
static_assert(std::same_as<iter::iterator_type, int*>);
static_assert(std::same_as<iter::value_type, int>);
static_assert(std::same_as<const_iter::value_type, int>);
static_assert(std::same_as<iter::difference_type, std::ptrdiff_t>);
static_assert(std::same_as<iter::pointer, int*>);
static_assert(std::same_as<const_iter::pointer, const int*>);
static_assert(std::same_as<iter::reference, int&>);
static_assert(std::same_as<const_iter::reference, const int&>);
static_assert(
    std::same_as<iter::iterator_category, std::random_access_iterator_tag>);
static_assert(
    std::same_as<iter::iterator_concept, std::contiguous_iterator_tag>);
static_assert(
    std::same_as<std::iter_rvalue_reference_t<string_iter>, std::string&&>);

// Iterator concepts: satisfied by
static_assert(std::input_iterator<iter>);
static_assert(std::forward_iterator<iter>);
static_assert(std::bidirectional_iterator<iter>);
static_assert(std::random_access_iterator<iter>);
static_assert(std::contiguous_iterator<iter>);
static_assert(std::contiguous_iterator<const_iter>);
static_assert(std::output_iterator<iter, int>);
static_assert(std::sentinel_for<iter, iter>);
static_assert(std::sized_sentinel_for<iter, iter>);
static_assert(std::sized_sentinel_for<const_iter, iter>);
static_assert(std::indirectly_swappable<iter>);
static_assert(std::totally_ordered<iter>);
static_assert(std::totally_ordered_with<iter, const_iter>);
static_assert(nothrow_forward_iterator<iter>);

// Iterator concepts: rejected, a const_iterator is not writable
static_assert(!std::output_iterator<const_iter, int>);

// Zero overhead over the raw pointer
static_assert(std::is_trivially_copyable_v<iter>);
static_assert(sizeof(iter) == sizeof(int*));

// Conversions: allowed from iterator to const_iterator and from the pointer
static_assert(std::is_nothrow_convertible_v<iter, const_iter>);
static_assert(std::is_nothrow_convertible_v<int*, iter>);

// Conversions: rejected where they would drop const
static_assert(!std::is_convertible_v<const_iter, iter>);
static_assert(!std::is_constructible_v<iter, const_iter>);
static_assert(!std::is_convertible_v<const int*, iter>);

// Range concepts of a subrange of wrappers
static_assert(std::ranges::contiguous_range<subrange>);
static_assert(std::ranges::sized_range<subrange>);
static_assert(std::ranges::common_range<subrange>);
static_assert(std::ranges::borrowed_range<subrange>);
static_assert(std::ranges::view<subrange>);

/// Traversal, difference and subscript all work in constant evaluation.
constexpr bool traverses_in_constant_evaluation() {
    constexpr int size = 5;
    std::array<int, size> array{ 1, 2, 3, 4, 5 };
    iter first(array.data());
    iter last(array.data() + array.size());

    int sum = 0;
    for (auto it = first; it != last; ++it) {
        sum += *it;
    }

    const bool sum_matches      = sum == 1 + 2 + 3 + 4 + 5;
    const bool distance_matches = last - first == size;
    const bool subscript_reads  = first[size - 1] == 5;
    const bool decrement_reads  = *(last - 1) == 5;
    return sum_matches && distance_matches && subscript_reads &&
           decrement_reads;
}
static_assert(traverses_in_constant_evaluation());

constexpr std::ptrdiff_t digits_size = 6;

/// { 0, 1, ..., 5 }: each element equals its index.
std::array<int, digits_size> make_digits() { return { 0, 1, 2, 3, 4, 5 }; }

void test_dereference_base_and_distance() {
    auto array = make_digits();
    iter first(array.data());
    iter last(array.data() + array.size());

    require(*first == 0);
    require(first.base() == array.data());
    require(last - first == digits_size);
    require(first - last == -digits_size);
    require(std::ranges::distance(first, last) == digits_size);
}

void test_increment_and_decrement() {
    auto array = make_digits();
    iter first(array.data());
    auto it = first;

    require(*++it == 1);
    require(*it++ == 1);
    require(*it == 2);
    require(*--it == 1);
    require(*it-- == 1);
    require(it == first);
}

void test_offsets_and_subscript() {
    auto array = make_digits();
    iter first(array.data());
    iter last(array.data() + array.size());
    auto it = first;

    it += 4;
    require(*it == 4);
    it -= 3;
    require(*it == 1);

    require(*(it + 2) == 3);
    require(*(2 + it) == 3);
    require(*(last - 1) == 5);
    require(it[3] == 4);
    require(first[-0] == 0);
    require((last - 2)[-1] == 3);
}

void test_writes_through_wrapper() {
    auto array = make_digits();
    iter first(array.data());

    *first   = 10;
    first[1] = 11;

    require(array[0] == 10);
    require(array[1] == 11);
}

void test_relational_operators() {
    std::array<int, 3> array{ 1, 2, 3 };
    iter first(array.data());
    iter second = first + 1;

    require(first == first);
    require(first != second);
    require(first < second);
    require(second > first);
    require(first <= first);
    require(first >= first);
    require((first <=> second) == std::strong_ordering::less);
    require((second <=> first) == std::strong_ordering::greater);
    require((first <=> first) == std::strong_ordering::equal);
}

void test_mixed_const_comparison_and_difference() {
    std::array<int, 3> array{ 1, 2, 3 };
    iter first(array.data());
    iter second       = first + 1;
    const_iter cfirst = first;
    const_iter csecond(array.data() + 1);

    require(first == cfirst);
    require(cfirst == first);
    require(first != csecond);
    require(csecond != first);
    require(first < csecond);
    require(csecond > first);
    require((cfirst <=> second) == std::strong_ordering::less);
    require(csecond - first == 1);
    require(first - csecond == -1);
}

void test_value_initialized_wrappers_compare_equal() {
    require(iter{} == iter{});
    require(iter{}.base() == nullptr);
}

void test_arrow_and_to_address() {
    std::array<std::string, 3> array{ "a", "bb", "ccc" };
    string_iter first(array.data());
    string_iter last(array.data() + array.size());

    require(first->size() == 1);
    require((first + 2)->size() == 3);
    require(std::to_address(first) == array.data());
    require(std::ranges::distance(first, last) == 3);
}

void test_iter_move_moves_element_out() {
    std::array<std::string, 3> array{ "a", "bb", "ccc" };
    string_iter first(array.data());

    std::string moved = std::ranges::iter_move(first + 1);

    require(moved == "bb");
    require(array[1].empty());
}

void test_iter_swap_exchanges_elements() {
    std::array<std::string, 3> array{ "a", "bb", "ccc" };
    string_iter first(array.data());

    std::ranges::iter_swap(first, first + 2);

    require(array[0] == "ccc");
    require(array[2] == "a");
}

/// Algorithm tests work on a permutation of 1..8.
constexpr std::size_t sample_size = 8;
/// Sum of 1..8.
constexpr int sample_sum = 36;

void test_ranges_sort_and_search() {
    std::array<int, sample_size> array{ 5, 3, 7, 1, 8, 2, 6, 4 };
    iter first(array.data());
    iter last(array.data() + array.size());
    std::ranges::subrange range(first, last);

    std::ranges::sort(range);

    require(std::ranges::is_sorted(array));
    require(std::ranges::binary_search(range, 6));
    const auto found = std::ranges::lower_bound(range, 4);
    require_or_return(found != range.end(), );
    require(*found == 4);
    require(std::accumulate(first, last, 0) == sample_sum);
    require(range.size() == sample_size);
    require(std::ranges::data(range) == array.data());
}

void test_classic_algorithm_uses_iterator_traits() {
    std::array<int, sample_size> array{ 1, 2, 3, 4, 5, 6, 7, 8 };
    iter first(array.data());
    iter last(array.data() + array.size());

    std::reverse(first, last);

    require(array.front() == 8);
    require(array.back() == 1);
}

void test_composes_with_views() {
    std::array<int, sample_size> array{ 8, 7, 6, 5, 4, 3, 2, 1 };
    std::ranges::subrange range(
        iter(array.data()), iter(array.data() + sample_size));

    auto evens = range | std::views::filter([](int value) {
                     return value % 2 == 0;
                 });

    auto doubled = range | std::views::reverse | std::views::take(3) |
                   std::views::transform([](int value) {
                       return value * 2;
                   });

    const std::array<int, 3> expected_doubled{ 2, 4, 6 };
    require(std::ranges::distance(evens) == 4);
    require(std::ranges::equal(doubled, expected_doubled));
}

void test_const_subrange_iterates() {
    std::array<int, sample_size> array{ 8, 7, 6, 5, 4, 3, 2, 1 };
    std::ranges::subrange range(
        iter(array.data()), iter(array.data() + sample_size));
    const auto& crange = range;

    int sum = 0;
    for (const int value : crange) {
        sum += value;
    }

    require(sum == sample_sum);
}

void test_empty_subrange() {
    std::array<int, 1> array{ 42 };
    iter first(array.data());

    std::ranges::subrange empty(first, first);

    require(empty.empty());
    require(empty.size() == 0);
    require(std::ranges::begin(empty) == std::ranges::end(empty));
}

} // namespace

int main() {
    test_dereference_base_and_distance();
    test_increment_and_decrement();
    test_offsets_and_subscript();
    test_writes_through_wrapper();
    test_relational_operators();
    test_mixed_const_comparison_and_difference();
    test_value_initialized_wrappers_compare_equal();
    test_arrow_and_to_address();
    test_iter_move_moves_element_out();
    test_iter_swap_exchanges_elements();
    test_ranges_sort_and_search();
    test_classic_algorithm_uses_iterator_traits();
    test_composes_with_views();
    test_const_subrange_iterates();
    test_empty_subrange();
    return neutron_test::result();
}
