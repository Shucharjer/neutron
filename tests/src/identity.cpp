#include <algorithm>
#include <array>
#include <cstddef>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <neutron/identity.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Id spaces, each with its own ids.
struct space_a {};
struct space_b {};

/// Distinct types for the concurrency test.
template <std::size_t>
struct tag {};

/// Requests the ids of `tag<Is>...` in `space_b`, in order.
template <std::size_t... Is>
std::vector<id_t> request_all(std::index_sequence<Is...>) {
    return { type_identity::identity<tag<Is>, space_b>()... };
}

/// Whether all elements of `values` are distinct.
template <typename Container>
bool all_distinct(Container values) {
    std::sort(values.begin(), values.end());
    return std::adjacent_find(values.begin(), values.end()) == values.end();
}

// Both generators return id_t without throwing.
static_assert(std::is_same_v<decltype(type_identity::identity<int>()), id_t>);
static_assert(
    std::is_same_v<decltype(non_type_identity::identity<int>()), id_t>);
static_assert(noexcept(type_identity::identity<int>()));
static_assert(noexcept(non_type_identity::identity<int>()));

// The id values depend on the request order: this must be the first test
// that requests type_identity ids.
void test_ids_are_dense_from_zero_in_request_order() {
    const id_t first  = type_identity::identity<int>();
    const id_t second = type_identity::identity<double>();
    const id_t third  = type_identity::identity<int*>();

    require(first == 0);
    require(second == 1);
    require(third == 2);
}

void test_repeated_requests_return_the_same_id() {
    const id_t first  = type_identity::identity<int>();
    const id_t second = type_identity::identity<double>();
    const id_t third  = type_identity::identity<int*>();

    require(type_identity::identity<int>() == first);
    require(type_identity::identity<double>() == second);
    require(type_identity::identity<int*>() == third);
}

void test_qualifiers_give_distinct_ids() {
    const std::array ids{
        type_identity::identity<char>(),
        type_identity::identity<const char>(),
        type_identity::identity<volatile char>(),
        type_identity::identity<char&>(),
        type_identity::identity<char&&>(),
        type_identity::identity<const char&>(),
    };

    require(all_distinct(ids));
}

void test_each_space_has_its_own_ids() {
    const id_t plain = type_identity::identity<long>();
    const id_t in_a  = type_identity::identity<long, space_a>();
    const id_t in_b  = type_identity::identity<long, space_b>();

    require(plain != in_a);
    require(plain != in_b);
    require(in_a != in_b);
    // void is the default space.
    require(type_identity::identity<long, void>() == plain);
    require(type_identity::identity<long, space_a>() == in_a);
}

// non_type_identity has not been used before, so its ids start from zero.
void test_generators_do_not_share_a_counter() {
    const id_t first  = non_type_identity::identity<int>();
    const id_t second = non_type_identity::identity<space_a>();

    require(first == 0);
    require(second == 1);
    require(non_type_identity::identity<int>() == first);
}

void test_concurrent_requests_agree_on_ids() {
    constexpr std::size_t threads = 8;
    constexpr std::size_t types   = 64;
    std::array<std::vector<id_t>, threads> results;

    {
        std::array<std::jthread, threads> workers;
        for (std::size_t i = 0; i != threads; ++i) {
            workers[i] = std::jthread([&result = results[i]] {
                result = request_all(std::make_index_sequence<types>());
            });
        }
    }

    // Every thread observes the same id for the same type.
    for (const auto& result : results) {
        require(result == results.front());
    }
    require(results.front().size() == types);
    require(all_distinct(results.front()));
}

} // namespace

int main() {
    test_ids_are_dense_from_zero_in_request_order();
    test_repeated_requests_return_the_same_id();
    test_qualifiers_give_distinct_ids();
    test_each_space_has_its_own_ids();
    test_generators_do_not_share_a_counter();
    test_concurrent_requests_agree_on_ids();
    return neutron_test::result();
}
