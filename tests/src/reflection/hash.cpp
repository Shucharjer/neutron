#include <concepts>
#include <neutron/metafn.hpp>
#include <neutron/reflection.hpp>

using namespace neutron;

template <typename...>
constexpr bool same = false;
template <typename T, typename... Others>
constexpr bool same<T, Others...> = (std::same_as<T, Others> && ...);

int main() {

    {
        static_assert(
            same<
                hash_list_t<type_list<char, int>>,
                hash_list_t<type_list<int, char>>>,
            "types sorted by hash should have same result");

        static_assert(
            same<
                hash_list_t<type_list<char, int, double>>,
                hash_list_t<type_list<char, double, int>>,
                hash_list_t<type_list<int, char, double>>,
                hash_list_t<type_list<int, double, char>>,
                hash_list_t<type_list<double, char, int>>,
                hash_list_t<type_list<double, int, char>>>,
            "types sorted by hash should have same result");
    }

    {
        using list_t =
            hash_list_t<type_list<char, int>>; // type_list<int, char>
        static_assert(same<
                      hash_sequence_t<type_list<char, int>>,
                      std::index_sequence<1, 0>>);
    }

    return 0;
}
