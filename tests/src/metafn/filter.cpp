#include <concepts>
#include <memory>
#include <string>
#include <type_traits>
#include <neutron/metafn.hpp>

using namespace neutron;

struct empty {};

template <auto V>
struct is_even : std::bool_constant<(V % 2 == 0)> {};

int main() {
    static_assert(
        std::same_as<
            type_list_filter_t<
                std::is_empty,
                type_list<
                    empty, std::allocator<std::byte>, int, char, std::string>>,
            type_list<empty, std::allocator<std::byte>>>);

    static_assert(std::same_as<
                  value_list_filt_t<is_even, value_list<1, 2, 3, 4>>,
                  value_list<2, 4>>);

    static_assert(std::same_as<
                  type_list_filt_nempty_t<
                      std::is_pointer, type_list<int, char>, void>,
                  void>);

    static_assert(std::same_as<
                  type_list_filt_nempty_t<
                      std::is_integral, type_list<int, char>, void>,
                  type_list<int, char>>);

    static_assert(std::same_as<
                  type_list_filt_tagged_t<
                      int,
                      type_list<
                          tagged_type_list<int, char>,
                          tagged_type_list<float, double>,
                          tagged_value_list<int, 1>>>,
                  type_list<
                      tagged_type_list<int, char>,
                      tagged_value_list<int, 1>>>);

    static_assert(std::same_as<
                  type_list_filt_tagged_nempty_t<
                      float, type_list<tagged_type_list<int, char>>, void>,
                  void>);

    return 0;
}
