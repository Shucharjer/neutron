#pragma once
#include <cstddef>
#include "neutron/detail/ecs/descriptor.hpp"

namespace neutron {

namespace internal {

template <std::size_t GroupId, typename... Worlds>
class run_env_for_group;

template <std::size_t GroupId, typename World>
class run_env_for_group<GroupId, World>;

template <typename World>
class run_env_for_individual;

} // namespace internal

template <typename Alloc, typename Impl>
class run_env;

template <typename Alloc, std::size_t GroupId, typename... Worlds>
class run_env<Alloc, internal::run_env_for_group<GroupId, Worlds...>> {
public:
    template <stage Stage>
    auto get_tasks();
};

template <typename Alloc, typename World>
class run_env<Alloc, internal::run_env_for_individual<World>> {
public:
    template <stage Stage>
    auto get_tasks();
};

} // namespace neutron
