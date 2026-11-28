#pragma once
#include "neutron/detail/ecs/compile-time/collect_params.hpp"
#include "neutron/detail/ecs/params/res.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <descriptor Desc>
using res_of = collect_params_of<Desc, res, true>;
}
