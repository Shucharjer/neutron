// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <cstdint>

namespace neutron {

enum class stage : std::uint8_t {
    prestartup,
    startup,
    poststartup,
    first,
    events,
    preupdate,
    update,
    postupdate,
    render,
    last,
    shutdown
};

} // namespace neutron
