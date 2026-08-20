// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <neutron/ecs.hpp>
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/compile-time/descriptor.hpp"
#include "neutron/detail/ecs/compile-time/queries.hpp"
#include "neutron/detail/ecs/core/world.hpp"

namespace neutron {

namespace internal {

template <std::size_t GroupId, typename... Worlds>
class run_env_for_group;

template <std::size_t GroupId, typename World>
class run_env_for_group<GroupId, World>;

template <typename World>
class run_env_for_individual;

} // namespace internal

template <typename Impl>
class run_env;

template <std::size_t GroupId, typename... Worlds>
class run_env<internal::run_env_for_group<GroupId, Worlds...>> {
public:
};

template <typename World>
class run_env<internal::run_env_for_individual<World>> {
public:
};

template <typename Alloc, typename Envs, auto... Worlds>
struct _run_envs_for_impl;
template <typename Alloc, typename... Envs>
struct _run_envs_for_impl<Alloc, std::tuple<Envs...>> {
    using type = std::tuple<Envs...>;
};
template <typename Alloc, typename... Envs, auto World, auto... Others>
struct _run_envs_for_impl<Alloc, std::tuple<Envs...>, World, Others...> {
    static constexpr auto policy          = get_execution_policy(World);
    static constexpr bool is_individual   = policy.is_individual;
    static constexpr std::size_t group_id = policy.id;

    using descriptor_t = std::remove_cvref_t<decltype(World)>;
    using world_t      = basic_world<descriptor_t, Alloc>;
    using env_t        = std::conditional_t<
               is_individual, internal::run_env_for_individual<world_t>,
               internal::run_env_for_group<group_id, world_t>>;

    using type = typename _run_envs_for_impl<
        Alloc, std::tuple<Envs..., env_t>, Others...>::type;
};

template <typename>
struct _run_envs_combine;

template <typename Alloc, auto... Worlds>
using run_envs_for = typename _run_envs_combine<
    typename _run_envs_for_impl<Alloc, std::tuple<>, Worlds...>::type>::type;

} // namespace neutron
