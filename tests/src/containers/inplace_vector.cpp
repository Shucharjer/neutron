#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
#include <neutron/inplace_vector.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

constexpr std::size_t capacity = 4;

/// Counts the elements visited by a range-for over `range`.
template <typename Range>
std::size_t count_visited(const Range& range) {
    std::size_t visited = 0;
    for (const auto& element : range) {
        static_cast<void>(element);
        ++visited;
    }
    return visited;
}

// A default-constructed vector is empty in a constant expression.
constexpr inplace_vector<int, capacity> default_ivec;
static_assert(default_ivec.size() == 0);

// emplace_back works in a constant expression and keeps insertion order.
constexpr auto emplaced_ivec = [] {
    inplace_vector<int, capacity> ivec;
    const std::vector<int> source{ 1, 2, 3, 4 };
    for (const auto value : source) {
        ivec.emplace_back(value);
    }
    return ivec;
}();
static_assert(emplaced_ivec[0] == 1);
static_assert(emplaced_ivec[1] == 2);
static_assert(emplaced_ivec[2] == 3);
static_assert(emplaced_ivec[3] == 4);

// The initializer-list constructor stores the elements in order.
constexpr inplace_vector<int, capacity> list_ivec{ 3, 4, 4, 8 };
static_assert(list_ivec.size() == 4);
static_assert(list_ivec[0] == 3);
static_assert(list_ivec[1] == 4);
static_assert(list_ivec[2] == 4);
static_assert(list_ivec[3] == 8);

#if defined(__cpp_lib_containers_ranges) &&                                    \
    __cpp_lib_containers_ranges >= 202202L

// The from_range constructor copies the range in order.
constexpr auto range_ivec = [] {
    std::array<int, capacity> source{ 1, 2, 3, 4 };
    inplace_vector<int, capacity> ivec{ std::from_range, std::move(source) };
    return ivec;
}();
static_assert(range_ivec[0] == 1);
static_assert(range_ivec[1] == 2);
static_assert(range_ivec[2] == 3);
static_assert(range_ivec[3] == 4);

#endif

// Zero-capacity vectors are empty constant expressions for any element type.
constexpr inplace_vector<int, 0> zero_int_ivec;
static_assert(zero_int_ivec.size() == 0);
constexpr inplace_vector<std::string, 0> zero_string_ivec;
static_assert(zero_string_ivec.size() == 0);

void test_non_trivial_element_default_constructs_empty() {
    inplace_vector<std::string, capacity> ivec;

    require(ivec.empty());
}

void test_zero_capacity_int_vector_iterates_nothing() {
    constexpr inplace_vector<int, 0> ivec;

    require(count_visited(ivec) == 0);
}

void test_zero_capacity_string_vector_iterates_nothing() {
    constexpr inplace_vector<std::string, 0> ivec;

    require(count_visited(ivec) == 0);
}

} // namespace

int main() {
    test_non_trivial_element_default_constructs_empty();
    test_zero_capacity_int_vector_iterates_nothing();
    test_zero_capacity_string_vector_iterates_nothing();
    return neutron_test::result();
}
