#pragma once
#include "neutron/detail/ecs/compile-time/collect_params.hpp"
#include "neutron/detail/ecs/params/local.hpp"

namespace neutron {

template <descriptor Desc>
using local_of = collect_params_with_sys_of<Desc, local, true>;

}
