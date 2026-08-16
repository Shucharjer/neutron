#include <array>
#include <map>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>
#include <neutron/concepts.hpp>
#include <neutron/reflection.hpp>

using namespace neutron;

template <typename T>
constexpr void test_traits() {
    constexpr type_traits traits = type_traits_of<T>();

    static_assert(traits.size == (std::is_empty_v<T> ? 0 : sizeof(T)));

    static_assert(traits.align == alignof(T));

    static_assert(
        traits.is_trivially_default_constructible ==
        std::is_trivially_default_constructible_v<T>);

    static_assert(
        traits.is_copy_constructible == std::is_copy_constructible_v<T>);

    static_assert(
        traits.is_trivially_copyable == std::is_trivially_copyable_v<T>);

    static_assert(
        traits.is_trivially_copy_assignable ==
        std::is_trivially_copy_assignable_v<T>);

    static_assert(
        traits.is_trivially_move_assignable ==
        std::is_trivially_move_assignable_v<T>);
    static_assert(
        traits.is_trivially_relocatible == trivially_relocatable<T>);
    static_assert(
        traits.is_trivially_destructible == std::is_trivially_destructible_v<T>);
    static_assert(traits.is_scalar == std::is_scalar_v<T>);
    static_assert(traits.is_object == std::is_object_v<T>);
    static_assert(traits.is_bounded_array == std::is_bounded_array_v<T>);
    static_assert(traits.is_enum == std::is_enum_v<T>);
    static_assert(traits.is_range == std::ranges::range<T>);
    static_assert(traits.is_map_like == map_like<T>);
    static_assert(traits.is_tuple_like == tuple_like<T>);
}

struct empty {};
enum class color : unsigned char { red, green };

int main() {
    test_traits<int>();
    test_traits<std::string>();
    test_traits<empty>();
    test_traits<color>();
    test_traits<int[3]>();
    test_traits<std::array<int, 3>>();
    test_traits<std::pair<int, double>>();
    test_traits<std::map<int, std::string>>();
    test_traits<std::vector<int>>();

    return 0;
}
