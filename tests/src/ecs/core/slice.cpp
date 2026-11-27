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
    std::span<T> span = { static_cast<T*>(*bufs[0]), slice.size() };
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

struct Index {
    using component_concept = component_tag;
    entity_t value{};
};

/// Every row of the slice holds its own entity, read through the slice.
bool slice_rows_match_entities(const slice<Index, std::string>& slice) {
    const auto* const indices = static_cast<Index*>(*slice.buffers()[0]);
    const auto* const names   = static_cast<std::string*>(*slice.buffers()[1]);
    for (size_t row = 0; row < slice.size(); ++row) {
        const auto entity = slice.entities()[row];
        if (indices[row].value != entity ||
            names[row] != std::to_string(entity)) {
            return false;
        }
    }
    return true;
}

void test_slice_taken_early_follows_later_changes() {
    archetype<> arche{ spread_type<Index, std::string> };
    archetype<> other{ arche, remove_components_t<std::string>{} };
    // Taken while empty; one of the two orders differs from the hash order.
    auto slice    = slice_of<Index, std::string>(arche);
    auto reversed = slice_of<std::string, Index>(arche);

    const auto count = static_cast<entity_t>(arche.capacity() * 4);
    for (entity_t entity = 1; entity <= count; ++entity) {
        arche.emplace(entity, Index{ entity }, std::to_string(entity));
    }
    require(slice.size() == count);
    require(slice_rows_match_entities(slice));
    require(*reversed.buffers()[0] == *slice.buffers()[1]);
    require(*reversed.buffers()[1] == *slice.buffers()[0]);

    arche.erase(1);
    arche.transfer(2, other);
    require(slice.size() == count - 2);
    require(slice_rows_match_entities(slice));

    arche.reserve(arche.capacity() * 2);
    require(slice_rows_match_entities(slice));

    arche.clear();
    require(slice.size() == 0);
}

} // namespace

int main() {
    test_slice_views_trivial_components();
    test_slice_views_heap_owning_components();
    test_slice_taken_early_follows_later_changes();
    return neutron_test::result();
}
