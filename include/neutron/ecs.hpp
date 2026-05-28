#pragma once
// IWYU pragma: begin_exports

//////////////////////////////////////////
// ECS concept
//////////////////////////////////////////

// Entity
#include "neutron/detail/ecs/entity.hpp"

// Component
#include "neutron/detail/ecs/bundle.hpp"
#include "neutron/detail/ecs/component.hpp"

// System

// auto fn(...);

//////////////////////////////////////////
// Forward declaration
//////////////////////////////////////////
#include "neutron/detail/ecs/fwd.hpp"

//////////////////////////////////////////
// System params
//////////////////////////////////////////

// query
#include "neutron/detail/ecs/anchor.hpp"
#include "neutron/detail/ecs/archetype.hpp"
#include "neutron/detail/ecs/querior.hpp"
#include "neutron/detail/ecs/query.hpp"
#include "neutron/detail/ecs/slice.hpp"
#include "neutron/detail/ecs/world_base.hpp"

// utility
#include "neutron/detail/ecs/world_accessor.hpp"

// commands, command_list
#include "neutron/detail/ecs/command_buffer.hpp"
#include "neutron/detail/ecs/command_list.hpp"
#include "neutron/detail/ecs/commands.hpp"

// data for single system
#include "neutron/detail/ecs/local.hpp"
#include "neutron/detail/ecs/systuple.hpp"

// data for a world
#include "neutron/detail/ecs/res.hpp"
#include "neutron/detail/ecs/resource.hpp"

// cross world data
#include "neutron/detail/ecs/global.hpp"

// type erased
#include "neutron/detail/ecs/meta_accessor.hpp"

// sync point
#include "neutron/detail/ecs/sync_point.hpp"

// world
#include "neutron/detail/ecs/world.hpp"

// snapshot
#include "neutron/detail/ecs/snapshot.hpp"

// startup
#include "neutron/detail/ecs/run.hpp"
#include "neutron/detail/ecs/run_env.hpp"
#include "neutron/detail/ecs/executor.hpp"

// IWYU pragma: end_exports
