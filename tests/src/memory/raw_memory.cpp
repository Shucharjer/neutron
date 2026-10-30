#include <algorithm>
#include <array>
#include <string>
#include <neutron/memory.hpp>

using namespace neutron;

using array_t        = std::array<char, 32>;
constexpr auto array = [] {
    std::string string = "this is a simple string";
    array_t array{};
    std::ranges::copy(string, array.begin());
    return array;
}();

int main() {

    {
        constexpr auto r = [] {
            array_t u;
            uninitialized_copy(array.begin(), array.end(), u.begin());
            return u;
        }();
        static_assert(array == r);
    }

    {
        constexpr auto r = [] {
            array_t u{};
            uninitialized_copy_n(array.begin(), 4, u.begin());
            return u;
        }();
        static_assert(r[0] == 't');
        static_assert(r[1] == 'h');
        static_assert(r[2] == 'i');
        static_assert(r[3] == 's');
        static_assert(r[4] == '\0');
    }

    {
        constexpr auto r = [] {
            array_t u{};
            uninitialized_fill(u.begin(), u.begin() + 4, '4');
            return u;
        }();
        static_assert(r[0] == '4');
        static_assert(r[1] == '4');
        static_assert(r[2] == '4');
        static_assert(r[3] == '4');
        static_assert(r[4] == '\0');
    }

    {
        constexpr auto r = [] {
            array_t u{};
            uninitialized_fill_n(u.begin(), 4, '4');
            return u;
        }();
        static_assert(r[0] == '4');
        static_assert(r[1] == '4');
        static_assert(r[2] == '4');
        static_assert(r[3] == '4');
        static_assert(r[4] == '\0');
    }

    {
        constexpr auto r = [] {
            array_t u;
            uninitialized_default_construct(u.begin(), u.end());
            return u;
        }();
        static_assert(r == array_t{});
    }

    {
        constexpr auto r = [] {
            array_t u;
            uninitialized_value_construct(u.begin(), u.end());
            return u;
        }();
        static_assert(r == array_t{});
    }

    {
        constexpr auto r = [] {
            array_t u;
            uninitialized_value_construct_n(u.begin(), 32);
            return u;
        }();
        static_assert(r == array_t{});
    }

    return 0;
}
