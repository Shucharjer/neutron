#include <concepts>
#include <tuple>
#include <neutron/metafn.hpp>
#include <neutron/reflection.hpp>
#include "require.hpp"

using namespace neutron;

int main() {
    // name_of yields the spelled type name; hash_of hashes that name.
    {
        static_assert(name_of<int>() == "int");
        static_assert(name_of<char>() == "char");
        static_assert(name_of<double>() == "double");
        static_assert(
            name_of<std::tuple<int, char>>() == "std::tuple<int, char>");
        static_assert(hash_of<int>() == internal::hash("int"));
        static_assert(hash_of<char>() == internal::hash("char"));
        static_assert(hash_of<int>() != hash_of<char>());
    }

    // hash_list_t sorts by hash, so the input order does not matter.
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

    // hash_sequence_t gives the source index of each sorted element.
    {
        using list_t = hash_list_t<type_list<char, int>>;
        static_assert(same<list_t, type_list<int, char>>);
        static_assert(same<
                      hash_sequence_t<type_list<char, int>>,
                      std::index_sequence<1, 0>>);
    }

    return neutron_test::result();
}
