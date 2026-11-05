#include <cstddef>
#include <iterator>
#include <string>
#include <utility>
#include <vector>
#include <neutron/small_vector.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

constexpr std::size_t inline_capacity = 4;

using vector_type = small_vector<int, inline_capacity>;
using const_iter  = vector_type::const_iterator;

/// Replaces the contents of `vec` with 0, 1, ..., count - 1 via push_back.
template <typename T, std::size_t N>
void fill_iota(small_vector<T, N>& vec, std::size_t count) {
    vec.clear();
    for (std::size_t i = 0; i < count; ++i) {
        vec.push_back(static_cast<T>(i));
    }
}

/// Returns a vector holding 1, 2, ..., count built via push_back.
vector_type make_counting(std::size_t count) {
    vector_type vec;
    for (std::size_t value = 1; value <= count; ++value) {
        vec.push_back(static_cast<int>(value));
    }
    return vec;
}

void test_default_constructor_uses_inline_capacity() {
    const vector_type smv;

    require(smv.size() == 0);
    require(smv.capacity() == inline_capacity);
}

void test_initializer_list_constructs_elements_not_count() {
    const vector_type smv{ 3, 4, 4 };

    require(smv.size() == 3);
    require(smv[0] == 3);
    require(smv[1] == 4);
    require(smv[2] == 4);
}

void test_count_constructor_value_initializes() {
    constexpr std::size_t count = 5;

    const small_vector<int, 8> smv(count);

    require(smv.size() == count);
    int sum = 0;
    for (const auto value : smv) {
        sum += value;
    }
    require(sum == 0);
}

void test_count_value_constructor_fills_beyond_inline_capacity() {
    constexpr std::size_t count = 6;
    constexpr int fill          = 7;

    const vector_type smv(count, fill);

    require(smv.size() == count);
    for (const auto value : smv) {
        require(value == fill);
    }
    require(smv.capacity() >= count);
}

void test_iterator_range_constructor_copies_range() {
    const std::vector<int> base{ 1, 2, 3, 4, 5 };

    const small_vector<int, 16> smv(base.begin(), base.end());

    require(smv.size() == base.size());
    for (std::size_t i = 0; i < base.size(); ++i) {
        require(smv[i] == base[i]);
    }
}

void test_push_back_within_inline_capacity() {
    vector_type vec;
    require(vec.size() == 0);
    require(vec.capacity() == inline_capacity);

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);

    require(vec.size() == inline_capacity);
    require(vec.front() == 1);
    require(vec.back() == 4);
}

void test_push_back_beyond_inline_capacity_doubles_capacity() {
    auto vec = make_counting(inline_capacity);

    vec.push_back(5);

    require(vec.size() == 5);
    require(vec.capacity() >= 2 * inline_capacity);
    require(vec[0] == 1);
    require(vec[4] == 5);
}

void test_pop_back_removes_last_element() {
    auto vec = make_counting(5);

    vec.pop_back();

    require(vec.size() == 4);
    require(vec.back() == 4);
}

void test_reserve_keeps_heap_elements() {
    constexpr std::size_t reserved = 64;
    // Spill to the heap first so reserve() reallocates heap storage.
    auto vec = make_counting(5);
    vec.pop_back();

    vec.reserve(reserved);

    require(vec.capacity() >= reserved);
    require(vec.size() == 4);
    require(vec[0] == 1);
    require(vec[3] == 4);
}

void test_resize_grow_keeps_existing_elements() {
    constexpr std::size_t new_size = 10;
    auto vec                       = make_counting(4);
    vec.reserve(64);

    vec.resize(new_size);

    require(vec.size() == new_size);
    require(vec[0] == 1);
    require(vec[3] == 4);
}

void test_resize_grow_fills_given_value() {
    constexpr std::size_t new_size = 12;
    constexpr int fill             = 9;
    auto vec                       = make_counting(4);
    vec.reserve(64);
    vec.resize(10);

    vec.resize(new_size, fill);

    require(vec.size() == new_size);
    require(vec[10] == fill);
    require(vec[11] == fill);
}

void test_resize_shrink_keeps_prefix() {
    auto vec = make_counting(12);

    vec.resize(2);

    require(vec.size() == 2);
    require(vec.back() == 2);
}

void test_insert_single_at_begin() {
    vector_type vec;
    fill_iota(vec, 5);

    vec.insert(static_cast<const_iter>(vec.begin()), 42);

    require(vec.size() == 6);
    require(vec.front() == 42);
}

void test_insert_count_in_middle_without_reallocation() {
    // Six elements on the heap leave room for two more.
    vector_type vec;
    fill_iota(vec, 6);

    vec.insert(static_cast<const_iter>(vec.begin() + 3), 2, 7);

    require(vec.size() == 8);
    require(vec[3] == 7);
    require(vec[4] == 7);
}

void test_insert_range_at_end_with_reallocation() {
    // A full heap buffer forces the range insert to reallocate.
    vector_type vec;
    fill_iota(vec, 8);
    int more[3] = { 9, 8, 6 };

    vec.insert(
        static_cast<const_iter>(vec.end()), std::begin(more), std::end(more));

    require(vec.size() == 11);
    require(vec.back() == 6);
}

void test_insert_initializer_list_in_middle() {
    vector_type vec;
    fill_iota(vec, 11);

    vec.insert(static_cast<const_iter>(vec.begin() + 2), { 1, 1, 1 });

    require(vec.size() == 14);
    require(vec[2] == 1);
    require(vec[3] == 1);
    require(vec[4] == 1);
}

void test_erase_single_element() {
    vector_type vec;
    fill_iota(vec, 14);

    vec.erase(static_cast<const_iter>(vec.begin()));

    require(vec.size() == 13);
}

void test_erase_range_returns_iterator_to_first_kept() {
    vector_type vec;
    fill_iota(vec, 13);

    auto iter = vec.erase(
        static_cast<const_iter>(vec.begin() + 1),
        static_cast<const_iter>(vec.begin() + 4));

    require(static_cast<std::size_t>(iter.base() - vec.data()) == 1);
    require(vec.size() == 10);
}

void test_assign_count_replaces_heap_contents() {
    constexpr std::size_t count = 3;
    constexpr int fill          = 5;
    vector_type vec;
    fill_iota(vec, 10);

    vec.assign(count, fill);

    require(vec.size() == count);
    for (const auto value : vec) {
        require(value == fill);
    }
}

void test_assign_iterator_range_grows_past_inline_capacity() {
    vector_type vec{ 5, 5, 5 };
    const std::vector<int> base{ 7, 7, 8, 8, 9 };

    vec.assign(base.begin(), base.end());

    require(vec.size() == base.size());
    for (std::size_t i = 0; i < base.size(); ++i) {
        require(vec[i] == base[i]);
    }
}

void test_assign_initializer_list_replaces_contents() {
    vector_type vec{ 7, 7, 8, 8, 9 };

    vec.assign({ 1, 2, 3, 4, 5, 6, 7 });

    require(vec.size() == 7);
    require(vec.front() == 1);
    require(vec.back() == 7);
}

void test_copy_constructs_inline_vector() {
    const small_vector<int, 8> source{ 1, 2, 3, 4 };

    const small_vector<int, 8> copy = source;

    require(copy.size() == source.size());
    require(copy[0] == 1);
    require(copy[3] == 4);
}

void test_move_constructs_inline_vector() {
    small_vector<int, 8> source{ 1, 2, 3, 4 };

    const small_vector<int, 8> moved = std::move(source);

    require(moved.size() == 4);
    require(moved[0] == 1);
    require(moved[3] == 4);
}

void test_copy_constructs_heap_vector() {
    constexpr std::size_t count = 16;
    vector_type source;
    fill_iota(source, count);

    const vector_type copy = source;

    require(copy.size() == count);
    require(copy[0] == 0);
    require(copy[15] == 15);
}

void test_move_constructs_heap_vector() {
    constexpr std::size_t count = 16;
    vector_type source;
    fill_iota(source, count);

    const vector_type moved = std::move(source);

    require(moved.size() == count);
    require(moved[5] == 5);
    require(moved[15] == 15);
}

void test_swap_exchanges_inline_and_heap_contents() {
    vector_type inline_vec{ 1, 2 };
    vector_type heap_vec{ 7, 8, 9, 10, 11 };

    inline_vec.swap(heap_vec);

    require(inline_vec.size() == 5);
    require(inline_vec[0] == 7);
    require(inline_vec.back() == 11);
    require(heap_vec.size() == 2);
    require(heap_vec[0] == 1);
    require(heap_vec[1] == 2);
}

void test_shrink_to_fit_returns_small_vector_to_inline_storage() {
    vector_type vec;
    fill_iota(vec, 20);
    require(vec.capacity() >= 20);
    vec.resize(3);

    vec.shrink_to_fit();

    require(vec.size() == 3);
    require(vec.capacity() == inline_capacity);
}

void test_shrink_to_fit_on_heap_never_grows_capacity() {
    vector_type vec;
    fill_iota(vec, 10);
    vec.resize(7);
    const auto old_capacity = vec.capacity();

    vec.shrink_to_fit();

    require(vec.capacity() <= old_capacity);
    require(vec.capacity() >= vec.size());
}

void test_emplace_back_forwards_constructor_arguments() {
    small_vector<std::string, inline_capacity> vec;

    vec.emplace_back("hello");
    vec.emplace_back(3, 'x');

    require(vec.size() == 2);
    require(vec[0] == "hello");
    require(vec[1] == std::string("xxx"));
}

void test_iterators_visit_every_element() {
    small_vector<std::string, inline_capacity> vec;
    vec.emplace_back("hello");
    vec.emplace_back(3, 'x');

    std::size_t count = 0;
    for (auto iter = vec.begin(); iter != vec.end(); ++iter) {
        ++count;
    }

    require(count == vec.size());
}

} // namespace

int main() {
    test_default_constructor_uses_inline_capacity();
    test_initializer_list_constructs_elements_not_count();
    test_count_constructor_value_initializes();
    test_count_value_constructor_fills_beyond_inline_capacity();
    test_iterator_range_constructor_copies_range();

    test_push_back_within_inline_capacity();
    test_push_back_beyond_inline_capacity_doubles_capacity();
    test_pop_back_removes_last_element();
    test_reserve_keeps_heap_elements();
    test_resize_grow_keeps_existing_elements();
    test_resize_grow_fills_given_value();
    test_resize_shrink_keeps_prefix();

    test_insert_single_at_begin();
    test_insert_count_in_middle_without_reallocation();
    test_insert_range_at_end_with_reallocation();
    test_insert_initializer_list_in_middle();
    test_erase_single_element();
    test_erase_range_returns_iterator_to_first_kept();
    test_assign_count_replaces_heap_contents();
    test_assign_iterator_range_grows_past_inline_capacity();
    test_assign_initializer_list_replaces_contents();

    test_copy_constructs_inline_vector();
    test_move_constructs_inline_vector();
    test_copy_constructs_heap_vector();
    test_move_constructs_heap_vector();
    test_swap_exchanges_inline_and_heap_contents();
    test_shrink_to_fit_returns_small_vector_to_inline_storage();
    test_shrink_to_fit_on_heap_never_grows_capacity();

    test_emplace_back_forwards_constructor_arguments();
    test_iterators_visit_every_element();
    return neutron_test::result();
}
