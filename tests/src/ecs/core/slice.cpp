#include "neutron/detail/ecs/core/slice.hpp"
#include <span>
#include <string>
#include <neutron/ecs.hpp>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

template <>
constexpr bool neutron::as_component<int> = true;
template <>
constexpr bool neutron::as_component<std::string> = true;

namespace {

template <typename T>
void require_slice_holds_every_emplaced_value(const T& val) {
    constexpr auto count = 1024;
    archetype<> archetype{ spread_type<T> };

    for (entity_t i = 1; i <= count; ++i) {
        archetype.emplace(i, val);
    }
    auto slice = slice_of<T>(archetype);

    require(archetype.size() == count);
    // The span below is sized by the slice, so it must not overrun the buffer.
    require_or_return(slice.size() == archetype.size(), );
    const auto& bufs  = slice.buffers();
    std::span<T> span = { reinterpret_cast<T*>(bufs[0]->get()), slice.size() };
    for (auto& data : span) {
        require(data == val);
    }
}

void test_slice_views_trivial_components() {
    require_slice_holds_every_emplaced_value<int>(32);
}

void test_slice_views_heap_owning_components() {
    require_slice_holds_every_emplaced_value<std::string>(
        "this is a std::string that needs heap allocation");
}

} // namespace

int main() {
    test_slice_views_trivial_components();
    test_slice_views_heap_owning_components();
    return neutron_test::result();
}
