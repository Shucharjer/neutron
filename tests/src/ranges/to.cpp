#include <list>
#include <map>
#include <ranges>
#include <vector>
#include <neutron/ranges.hpp>
#include "require.hpp"

using namespace neutron;

int main() {
    {
        std::map<int, int> map = {
            {   1,   5 },
            { 534,   2 },
            {  34, 756 },
            { 423,  87 },
            { 756,  15 }
        };
        auto vector = ranges::to<std::vector>(map | std::views::values);
        auto vit    = vector.begin();
        auto mit    = map.begin();
        for (; vit != vector.end() && mit != map.end(); ++vit, ++mit) {
            require_or_return(*vit == mit->second, 1);
        }

        auto transformed = vector | std::views::transform([&](const auto& val) {
                               return std::to_string(val + 1);
                           });
    }

    {
        std::array<char, sizeof("word")> array = { 'w', 'o', 'r', 'd', '\0' };
        {
            auto string = ranges::to<std::string>(array);
            auto sit    = string.begin();
            auto* ait   = array.begin();
            for (; sit != string.end() && ait != array.end(); ++sit, ++ait) {
                require_or_return(*sit == *ait, 1);
            }
        }

        {
            auto list = ranges::to<std::list>(array);
            auto lit  = list.begin();
            auto* ait = array.begin();
            for (; lit != list.end() && ait != array.end(); ++lit, ++ait) {
                require_or_return(*lit == *ait, 1);
            }
        }
    }

    return 0;
}
