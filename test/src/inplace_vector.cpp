#include <stdexcept>
#include <string>
#include <vector>
#include <neutron/inplace_vector.hpp>

using namespace neutron;

void test_trivial() {
    {
        constexpr inplace_vector<int, 4> ivec;
        static_assert(ivec.size() == 0);
    }

    {
        constexpr auto ivec = [] {
            inplace_vector<int, 4> ivec;
            std::vector<int> vec{ 1, 2, 3, 4 };
            for (auto val : vec) {
                ivec.emplace_back(val);
            }
            return ivec;
        }();
        static_assert(ivec[0] == 1);
        static_assert(ivec[1] == 2);
        static_assert(ivec[2] == 3);
        static_assert(ivec[3] == 4);
    }

    {
        constexpr inplace_vector<int, 4> ivec{ 3, 4, 4, 8 };
        static_assert(ivec.size() == 4);
        static_assert(ivec[0] == 3);
        static_assert(ivec[1] == 4);
        static_assert(ivec[2] == 4);
        static_assert(ivec[3] == 8);
    }

#if defined(__cpp_lib_containers_ranges) &&                                    \
    __cpp_lib_containers_ranges >= 202202L

    {
        constexpr auto ivec = [] {
            std::vector<int> vec{ 1, 2, 3, 4 };
            inplace_vector<int, 4> ivec{ std::from_range, std::move(vec) };
            return vec;
        }();
        static_assert(ivec[0] == 1);
        static_assert(ivec[1] == 2);
        static_assert(ivec[2] == 3);
        static_assert(ivec[3] == 4);
    }

#endif

    { constexpr inplace_vector<int, 4> ivec; }
}

void test_non_trivial() {
    { inplace_vector<std::string, 4> ivec; }
}

void test_zero_sized() {
    {
        constexpr inplace_vector<int, 0> ivec;
        static_assert(ivec.size() == 0);
    }

    {
        constexpr inplace_vector<std::string, 0> ivec;
        static_assert(ivec.size() == 0);
    }

    {
        constexpr inplace_vector<int, 0> ivec;
        for (auto val : ivec) {
            throw std::logic_error(
                "zero sized inplace_vector should not have element");
        }
    }

    {
        constexpr inplace_vector<std::string, 0> ivec;
        for (const auto& val : ivec) {
            throw std::logic_error(
                "zero sized inplace_vector should not have element");
        }
    }
}

int main() {
    try {
        test_trivial();
        test_non_trivial();
        test_zero_sized();
    } catch (...) {
        return -1;
    }
    return 0;
}
