// type_traits_of<T>() field-by-field checks live in traits.cpp; this file
// covers traits_list_of and the packed layout.
#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>
#include <neutron/metafn.hpp>
#include <neutron/reflection.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

struct empty {};

struct alignas(64) over_aligned {
    int value;
};

constexpr bool same_traits(type_traits lhs, type_traits rhs) noexcept {
    return lhs.size == rhs.size && lhs.align == rhs.align &&
           lhs.is_trivially_default_constructible ==
               rhs.is_trivially_default_constructible &&
           lhs.is_copy_constructible == rhs.is_copy_constructible &&
           lhs.is_trivially_copyable == rhs.is_trivially_copyable &&
           lhs.is_trivially_copy_assignable ==
               rhs.is_trivially_copy_assignable &&
           lhs.is_trivially_move_assignable ==
               rhs.is_trivially_move_assignable &&
           lhs.is_trivially_relocatible == rhs.is_trivially_relocatible &&
           lhs.is_trivially_destructible == rhs.is_trivially_destructible &&
           lhs.is_scalar == rhs.is_scalar && lhs.is_object == rhs.is_object &&
           lhs.is_bounded_array == rhs.is_bounded_array &&
           lhs.is_enum == rhs.is_enum && lhs.is_range == rhs.is_range &&
           lhs.is_map_like == rhs.is_map_like &&
           lhs.is_tuple_like == rhs.is_tuple_like;
}

// The traits are packed into a single trivially copyable word.
static_assert(sizeof(type_traits) == sizeof(std::uint64_t));
static_assert(std::is_trivially_copyable_v<type_traits>);

// Over-aligned types keep their alignment in the 16-bit field.
static_assert(type_traits_of<over_aligned>().align == 64);
static_assert(type_traits_of<over_aligned>().size == 64);

template <typename List, typename... Ts>
constexpr bool check_list() noexcept {
    constexpr auto list = traits_list_of<List>();
    static_assert(list.size() == sizeof...(Ts));
    std::size_t index = 0;
    return (same_traits(list[index++], type_traits_of<Ts>()) && ...);
}

// The list holds type_traits_of<T>() for every element type.
static_assert(traits_list_of<std::tuple<>>().empty());
static_assert(check_list<std::tuple<int>, int>());
static_assert(check_list<
              std::tuple<int, std::string, empty, over_aligned>, int,
              std::string, empty, over_aligned>());
static_assert(check_list<
              type_list<std::vector<int>, double, char>, std::vector<int>,
              double, char>());

// Order is preserved, not sorted.
static_assert(traits_list_of<std::tuple<char, double>>()[0].size == 1);
static_assert(traits_list_of<std::tuple<char, double>>()[1].size == 8);
static_assert(traits_list_of<std::tuple<empty, int>>()[0].size == 0);

void test_traits_list_is_usable_at_runtime() {
    const auto list = traits_list_of<std::tuple<int, std::string>>();

    require_or_return(list.size() == 2, );
    require(list[0].size == sizeof(int));
    require(list[0].is_scalar);
    require_false(list[1].is_scalar);
    require(list[1].is_range);
}

} // namespace

int main() {
    test_traits_list_is_usable_at_runtime();
    return neutron_test::result();
}
