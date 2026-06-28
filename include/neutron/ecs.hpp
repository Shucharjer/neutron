#pragma once
// IWYU pragma: begin_exports

#include "neutron/detail/ecs/fwd.hpp"

#include "neutron/detail/ecs/concepts/bundle.hpp"
#include "neutron/detail/ecs/concepts/component.hpp"
#include "neutron/detail/ecs/concepts/entity.hpp"
#include "neutron/detail/ecs/concepts/resource.hpp"

#include "neutron/detail/ecs/utility/anchor.hpp"
#include "neutron/detail/ecs/utility/systuple.hpp"

#include "neutron/detail/ecs/core/archetype.hpp"
#include "neutron/detail/ecs/core/command_buffer.hpp"
#include "neutron/detail/ecs/core/querior.hpp"
#include "neutron/detail/ecs/core/slice.hpp"
#include "neutron/detail/ecs/core/world_accessor.hpp"
#include "neutron/detail/ecs/core/world_base.hpp"

#include "neutron/detail/ecs/compile-time/queries.hpp"

#include "neutron/detail/ecs/core/world.hpp"

#include "neutron/detail/ecs/params/command_list.hpp"
#include "neutron/detail/ecs/params/commands.hpp"
#include "neutron/detail/ecs/params/global.hpp"
#include "neutron/detail/ecs/params/local.hpp"
#include "neutron/detail/ecs/params/meta_accessor.hpp"
#include "neutron/detail/ecs/params/query.hpp"
#include "neutron/detail/ecs/params/res.hpp"
#include "neutron/detail/ecs/params/sync_point.hpp"

#include "neutron/detail/ecs/snapshot.hpp"

#include "neutron/detail/ecs/executor.hpp"
#include "neutron/detail/ecs/runtime/run.hpp"
#include "neutron/detail/ecs/runtime/run_env.hpp"

// IWYU pragma: end_exports
