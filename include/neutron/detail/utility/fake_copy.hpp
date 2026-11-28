#pragma once

namespace neutron {

ATOM_EXPORT constexpr auto fake_copy(auto&& value) {
    auto val{ value };
    return static_cast<decltype(val)*>(nullptr);
};

} // namespace neutron
