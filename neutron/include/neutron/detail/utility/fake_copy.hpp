#pragma once

namespace neutron {

constexpr auto fake_copy(auto&& value) -> decltype([] {
    auto val{ value };
    return static_cast<decltype(val)*>(nullptr);
});

} // namespace neutron
