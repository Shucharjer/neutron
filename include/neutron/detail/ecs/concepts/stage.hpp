// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <cstdint>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT enum class stage : std::uint8_t {
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
