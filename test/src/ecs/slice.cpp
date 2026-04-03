#include "neutron/detail/ecs/slice.hpp"
#include <string>
#include <neutron/ecs.hpp>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

template <>
constexpr bool neutron::as_component<int> = true;
template <>
constexpr bool neutron::as_component<std::string> = true;

template <typename T>
int test_slice(const T& val) {
    constexpr auto count = 1024;

    archetype<> archetype{ spread_type<T> };
    for (entity_t i = 1; i <= count; ++i) {
        archetype.emplace(i);
    }
    require_or_return(archetype.size() == count, 1);
    auto slice = slice_of<int>(archetype);
    require_or_return(slice.size() == archetype.size(), 1);

    const auto& bufs  = slice.buffers();
    std::span<T> span = { reinterpret_cast<T*>(bufs[0]->get()), slice.size() };
    for (auto& data : span) {
        require_or_return(data == val, 1);
    }

    return 0;
}

int main() {
    require_or_return(test_slice<int>(32), 1);
    require_or_return(
        test_slice<std::string>(
            "this is a std::string that needs heap allocation"),
        1);

    return 0;
}
