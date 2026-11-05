#include <cstddef>
#include <cstdint>
#include <neutron/detail/reflection/offset_helper.hpp>
#include "require.hpp"

using neutron::_refl::_offset_mapping;
using neutron::_refl::offset_helper;

namespace {

std::size_t offset_of_member(std::uint8_t offset_helper::* member) {
    alignas(offset_helper) std::byte storage[sizeof(offset_helper)]{}; // NOLINT
    auto* helper = reinterpret_cast<offset_helper*>(storage);
    return static_cast<std::size_t>(
        reinterpret_cast<std::byte*>(&(helper->*member)) - storage);
}

void test_every_offset_maps_to_member_at_that_offset() {
    // Includes offsets past the first mapping table.
    for (std::size_t offset = 0; offset != sizeof(offset_helper); ++offset) {
        require(offset_of_member(_offset_mapping(offset)) == offset);
    }
}

} // namespace

int main() {
    test_every_offset_maps_to_member_at_that_offset();
    return neutron_test::result();
}
