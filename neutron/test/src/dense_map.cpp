#include <cstdint>
#include <memory_resource>
#include <neutron/dense_map.hpp>
#include "require.hpp"

void test_dense_map();
void test_dense_map_pmr();

int main() {
    test_dense_map();
    test_dense_map_pmr();
    return 0;
}

using namespace neutron;

void test_dense_map() {
    // default ctor
    {
        constexpr dense_map<id_t, id_t> map;

        constexpr auto result = [] {
            dense_map<id_t, id_t> map;
            return 0;
        }();
    }

    // ctor (initializer list)
    {
        dense_map<id_t, id_t> map{
            { 3, 3 }
        };

        constexpr auto result = [] {
            dense_map<id_t, id_t> map{
                { 2, 2 },
                { 3, 3 }
            };
            return map.size();
        }();
        static_assert(result == 2);
    }

    // emplace
    {
        constexpr auto result = [] {
            dense_map<id_t, id_t> map;
            auto [iter, _] = map.emplace(3, 3); // NOLINT: unused
            return std::make_tuple(map.size(), iter->first, iter->second);
        }();
        using std::get;
        static_assert(get<0>(result) == 1);
        static_assert(get<1>(result) == 3);
        static_assert(get<2>(result) == 3);
    }

    // erase
    {
        constexpr auto result = [] {
            dense_map<id_t, id_t> map;
            map.emplace(2, 3);
            map.emplace(3, 3);
            map.erase(2);
            return map.size();
        }();
        static_assert(result == 1);
    }

    // contains
    {
        constexpr auto result = [] {
            dense_map<id_t, id_t> map;
            auto first = map.contains(1);
            map.emplace(1, 1);
            auto second = map.contains(1);
            return std::make_pair(first, second);
        }();
        static_assert(!result.first);
        static_assert(result.second);
    }

    // traverse
    {
        constexpr auto result = [] {
            dense_map<id_t, id_t> map{
                { 1, 1 },
                { 2, 2 },
                { 4, 2 }
            };

            for (auto& [first, second] : map) {}
            for (auto iter = map.begin(); iter != map.end(); ++iter) {}
            for (auto iter = map.cbegin(); iter != map.cend(); ++iter) {}
            for (auto iter = map.rbegin(); iter != map.rend(); ++iter) {}
            for (auto iter = map.crbegin(); iter != map.crend(); ++iter) {}

            return 0;
        }();
    }
}

// pmr::dense_map
void test_dense_map_pmr() {
    auto pool  = std::pmr::unsynchronized_pool_resource{};
    auto alloc = std::pmr::polymorphic_allocator<>{ &pool };

    // default ctor
    { pmr::dense_map<id_t, id_t> map; }

    // alloc
    {
        { pmr::dense_map<id_t, id_t> map{ alloc }; }
        { pmr::dense_map<id_t, id_t> map{ std::allocator_arg, alloc }; }
    }

    // ctor (il)
    {
        {
            pmr::dense_map<id_t, id_t> map{
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

        {
            std::initializer_list<std::pair<id_t, id_t>> ilist = {
                { 0, 1 },
                { 1, 1 }
            };
            pmr::dense_map<id_t, id_t> map{ std::allocator_arg, alloc, ilist };
            require(map.size() == 2);
            require(map.contains(0));
            require(map.at(0) == 1);
            require(map.contains(1));
            require(map.at(1) == 1);
        }
    }

    // {
    //     std::tuple t1 = { std::allocator_arg, std::allocator<int>{}, 0, 0 };
    //     std::tuple t2 = { 1 };
    //     std::pair<std::vector<int>, int> p{ std::piecewise_construct, t1, t2
    //     };
    // }
    std::vector<int> vec;
}
