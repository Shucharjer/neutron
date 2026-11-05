#include <cstddef>
#include <initializer_list>
#include <memory>
#include <memory_resource>
#include <tuple>
#include <utility>
#include <neutron/shift_map.hpp>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

using map_type     = shift_map<id_t, id_t>;
using pmr_map_type = pmr::shift_map<id_t, id_t>;

/// Iteration counts of a map through each iterator flavour.
struct traversal_counts {
    std::size_t range_for;
    std::size_t forward;
    std::size_t const_forward;
    std::size_t reverse;
    std::size_t const_reverse;
};

// A default-constructed map is empty in a constant expression.
static_assert([] {
    map_type map;
    return map.empty() && map.size() == 0;
}());

// The initializer-list constructor inserts every pair.
static_assert([] {
    map_type map{
        { 2, 2 },
        { 3, 3 }
    };
    return map.size();
}() == 2);

// emplace returns an iterator to the inserted pair.
constexpr auto emplace_result = [] {
    map_type map;
    auto [iter, _] = map.emplace(3, 3); // NOLINT: unused
    return std::make_tuple(map.size(), iter->first, iter->second);
}();
static_assert(std::get<0>(emplace_result) == 1);
static_assert(std::get<1>(emplace_result) == 3);
static_assert(std::get<2>(emplace_result) == 3);

// erase by key removes exactly that key.
static_assert([] {
    map_type map;
    map.emplace(2, 3);
    map.emplace(3, 3);
    map.erase(2);
    return map.size();
}() == 1);

// contains reflects a preceding emplace.
constexpr auto contains_result = [] {
    map_type map;
    const auto before = map.contains(1);
    map.emplace(1, 1);
    const auto after = map.contains(1);
    return std::make_pair(before, after);
}();
static_assert(!contains_result.first);
static_assert(contains_result.second);

// Every iterator flavour visits each element once in a constant expression.
constexpr auto traversal = [] {
    map_type map{
        { 1, 1 },
        { 2, 2 },
        { 4, 2 }
    };

    traversal_counts counts{};
    for ([[maybe_unused]] auto& [key, value] : map) {
        ++counts.range_for;
    }
    for (auto iter = map.begin(); iter != map.end(); ++iter) {
        ++counts.forward;
    }
    for (auto iter = map.cbegin(); iter != map.cend(); ++iter) {
        ++counts.const_forward;
    }
    for (auto iter = map.rbegin(); iter != map.rend(); ++iter) {
        ++counts.reverse;
    }
    for (auto iter = map.crbegin(); iter != map.crend(); ++iter) {
        ++counts.const_reverse;
    }
    return counts;
}();
constexpr std::size_t traversal_size = 3;
static_assert(traversal.range_for == traversal_size);
static_assert(traversal.forward == traversal_size);
static_assert(traversal.const_forward == traversal_size);
static_assert(traversal.reverse == traversal_size);
static_assert(traversal.const_reverse == traversal_size);

void test_initializer_list_constructs_at_runtime() {
    const map_type map{
        { 3, 3 }
    };

    require(map.size() == 1);
    require(map.contains(3));
}

void test_pmr_default_constructs_empty() {
    const pmr_map_type map;

    require(map.empty());
}

void test_pmr_allocator_constructors_construct_empty() {
    auto pool  = std::pmr::unsynchronized_pool_resource{};
    auto alloc = std::pmr::polymorphic_allocator<>{ &pool };

    const pmr_map_type plain{ alloc };
    const pmr_map_type tagged{ std::allocator_arg, alloc };

    require(plain.empty());
    require(tagged.empty());
}

void test_pmr_initializer_list_with_trailing_allocator() {
    auto pool  = std::pmr::unsynchronized_pool_resource{};
    auto alloc = std::pmr::polymorphic_allocator<>{ &pool };

    const pmr_map_type map{
        { { 0, 1 }, { 1, 1 }, { 2, 4 } },
        alloc
    };

    require(map.size() == 3);
    require(map.contains(0));
    require(map.at(0) == 1);
    require(map.contains(1));
    require(map.at(1) == 1);
    require(map.contains(2));
    require(map.at(2) == 4);
}

void test_pmr_initializer_list_with_allocator_arg() {
    auto pool  = std::pmr::unsynchronized_pool_resource{};
    auto alloc = std::pmr::polymorphic_allocator<>{ &pool };
    const std::initializer_list<std::pair<id_t, id_t>> ilist = {
        { 0, 1 },
        { 1, 1 }
    };

    const pmr_map_type map{ std::allocator_arg, alloc, ilist };

    require(map.size() == 2);
    require(map.contains(0));
    require(map.at(0) == 1);
    require(map.contains(1));
    require(map.at(1) == 1);
}

} // namespace

int main() {
    test_initializer_list_constructs_at_runtime();
    test_pmr_default_constructs_empty();
    test_pmr_allocator_constructors_construct_empty();
    test_pmr_initializer_list_with_trailing_allocator();
    test_pmr_initializer_list_with_allocator_arg();
    return neutron_test::result();
}
