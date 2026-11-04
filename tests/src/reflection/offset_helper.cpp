#include <cstddef>
#include <cstdint>
#include <neutron/detail/reflection/offset_helper.hpp>

using neutron::_refl::_offset_mapping;
using neutron::_refl::offset_helper;

namespace {

std::size_t offset_of_member(std::uint8_t offset_helper::* member) {
    alignas(offset_helper) std::byte storage[sizeof(offset_helper)]{}; // NOLINT
    auto* helper = reinterpret_cast<offset_helper*>(storage);
    return static_cast<std::size_t>(
        reinterpret_cast<std::byte*>(&(helper->*member)) - storage);
}

} // namespace

int main() {
    // Every offset, including those past the first mapping table, maps back
    // to the member at that offset.
    for (std::size_t offset = 0; offset != sizeof(offset_helper); ++offset) {
        if (offset_of_member(_offset_mapping(offset)) != offset) {
            return 1;
        }
    }
    return 0;
}
