// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <chrono>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/compile-time/descriptor.hpp"
#include "neutron/detail/ecs/compile-time/queries.hpp"
#include "neutron/detail/ecs/core/world.hpp"
#include "neutron/detail/execution/core.hpp"
#include "neutron/detail/lockfree/hardware_destructive_interference_size.hpp"
#include "neutron/detail/metafn/definition.hpp"

namespace neutron {

template <typename T>
concept run_env = requires(T& env) {
    typename T::policy_type;
    env.template get<0>();
};

template <std::size_t GroupId, typename... Worlds>
class run_env_for_group;

template <std::size_t GroupId, typename World>
class alignas(hdi_size) run_env_for_group<GroupId, World> {
public:
    using policy_type     = _group_t<GroupId>;
    using descriptor_type = typename World::descriptor_type;
    using allocator_type  = typename World::allocator_type;
    using archetype       = typename World::archetype;
    using command_buffer  = typename World::command_buffer;

    template <typename Alloc = allocator_type>
    run_env_for_group(const Alloc& alloc = {}) : world_(alloc) {}

    template <std::size_t Index>
    requires(Index == 0)
    auto& get() & noexcept {
        return world_;
    }

    template <stage Stage, execution::scheduler Scheduler>
    ATOM_NODISCARD execution::sender auto
        run(Scheduler scheduler, _time_point_t time) & {
        return world_.template run<Stage>(scheduler, time);
    }

private:
    World world_;
};

template <std::size_t GroupId, typename World, typename... Others>
requires(sizeof...(Others) != 0)
class alignas(hdi_size) run_env_for_group<GroupId, World, Others...> {
public:
    using policy_type     = _group_t<GroupId>;
    using descriptor_type = typename World::descriptor_type;
    using allocator_type  = typename World::allocator_type;
    using archetype       = typename World::archetype;
    using command_buffer  = typename World::command_buffer;

    template <typename Alloc = allocator_type>
    explicit run_env_for_group(const Alloc& alloc = {})
        : run_env_for_group(
              std::index_sequence_for<World, Others...>{}, alloc) {}

    template <std::size_t Index>
    auto& get() & noexcept {
        return worlds_.template get<Index>();
    }

    template <stage Stage, execution::scheduler Scheduler>
    ATOM_NODISCARD execution::sender auto
        run(Scheduler scheduler, _time_point_t time) & {
        return _run<Stage, 0>(scheduler, time);
    }

private:
    template <stage Stage, std::size_t Index, execution::scheduler Scheduler>
    auto _run(Scheduler scheduler, _time_point_t time) {
        auto sender = get<Index>().template run<Stage>(scheduler, time);
        if constexpr (Index == sizeof...(Others)) {
            return sender;
        } else {
            return std::move(sender) |
                   execution::let_value([this, scheduler, time] {
                       return _run<Stage, Index + 1>(scheduler, time);
                   });
        }
    }

    template <std::size_t... Is, typename Alloc>
    run_env_for_group(std::index_sequence<Is...>, const Alloc& alloc)
        : worlds_((static_cast<void>(Is), alloc)...) {}

    shared_tuple<World, Others...> worlds_;
};

template <typename World>
class alignas(hdi_size) run_env_for_individual {
public:
    using policy_type     = _individual_t;
    using descriptor_type = typename World::descriptor_type;
    using allocator_type  = typename World::allocator_type;
    using archetype       = typename World::archetype;
    using command_buffer  = typename World::command_buffer;

    template <typename Alloc = allocator_type>
    explicit run_env_for_individual(const Alloc& alloc = {}) : world_(alloc) {}

    template <std::size_t Index>
    requires(Index == 0)
    auto& get() & noexcept {
        return world_;
    }

    template <stage Stage, execution::scheduler Scheduler>
    ATOM_NODISCARD execution::sender auto
        run(Scheduler scheduler, _time_point_t time) & {
        return world_.template run<Stage>(scheduler, time);
    }

private:
    World world_;
};

template <typename Alloc, typename Envs, auto... Worlds>
struct _run_envs_for_impl;
template <typename Alloc, typename... Envs>
struct _run_envs_for_impl<Alloc, type_list<Envs...>> {
    using type = type_list<Envs...>;
};
template <typename Alloc, typename... Envs, auto World, auto... Others>
struct _run_envs_for_impl<Alloc, type_list<Envs...>, World, Others...> {
    static constexpr auto policy          = get_execution_policy(World);
    static constexpr bool is_individual   = policy.is_individual;
    static constexpr std::size_t group_id = policy.id;

    using descriptor_t = std::remove_cvref_t<decltype(World)>;
    using world_t      = basic_world<descriptor_t, Alloc>;
    using env_t        = std::conditional_t<
               is_individual, run_env_for_individual<world_t>,
               run_env_for_group<group_id, world_t>>;

    using type = typename _run_envs_for_impl<
        Alloc, type_list<Envs..., env_t>, Others...>::type;
};

template <typename>
struct _run_envs_combine;

template <typename, typename>
struct _run_envs_combine_impl;

template <template <typename...> typename Tmp, typename... Result>
struct _run_envs_combine_impl<Tmp<>, Tmp<Result...>> {
    using type = Tmp<Result...>;
};

template <typename>
struct _run_env_combine;

template <
    template <typename...> typename Tmp, std::size_t GroupId,
    typename... Worlds>
struct _run_env_combine<Tmp<run_env_for_group<GroupId, Worlds>...>> {
    using type = run_env_for_group<GroupId, Worlds...>;
};

template <
    template <typename...> typename Tmp, std::size_t GroupId, typename World,
    typename... Others, typename... Result>
struct _run_envs_combine_impl<
    Tmp<run_env_for_group<GroupId, World>, Others...>, Tmp<Result...>> {
    using policy_type = _group_t<GroupId>;
    template <typename T>
    using filter = std::is_same<typename T::policy_type, policy_type>;
    template <typename T>
    using neg    = std::negation<filter<T>>;
    using envs_t = type_list_filter_t<
        filter, Tmp<run_env_for_group<GroupId, World>, Others...>>;
    using env_t    = typename _run_env_combine<envs_t>::type;
    using others_t = type_list_filter_t<
        neg, Tmp<run_env_for_group<GroupId, World>, Others...>>;
    using result_t = Tmp<Result..., env_t>;
    using type     = typename _run_envs_combine_impl<others_t, result_t>::type;
};

template <
    template <typename...> typename Tmp, typename World, typename... Others,
    typename... Result>
struct _run_envs_combine_impl<
    Tmp<run_env_for_individual<World>, Others...>, Tmp<Result...>> {
    using others_t = Tmp<Others...>;
    using result_t = Tmp<Result..., run_env_for_individual<World>>;
    using type     = typename _run_envs_combine_impl<others_t, result_t>::type;
};

template <template <typename...> typename Tmp, typename... Envs>
struct _run_envs_combine<Tmp<Envs...>> {
    using type = typename _run_envs_combine_impl<Tmp<Envs...>, Tmp<>>::type;
};

template <typename... Envs>
class run_envs;

template <>
class run_envs<> : public std::tuple<> {
public:
    using allocator_type = std::allocator<std::byte>;

    template <typename Alloc = allocator_type>
    explicit run_envs(const Alloc& = {}) {}

    template <stage Stage, execution::scheduler Scheduler>
    ATOM_NODISCARD execution::sender auto
        run(Scheduler, _time_point_t = std::chrono::system_clock::now()) & {
        return execution::just();
    }
};

template <typename Env, typename... Others>
class run_envs<Env, Others...> : public std::tuple<Env, Others...> {
public:
    using allocator_type = typename Env::allocator_type;

    template <typename Alloc = allocator_type>
    explicit run_envs(const Alloc& alloc = {})
        : run_envs(std::index_sequence_for<Env, Others...>{}, alloc) {}

    template <stage Stage, execution::scheduler Scheduler>
    ATOM_NODISCARD execution::sender auto
        run(Scheduler scheduler,
            _time_point_t time = std::chrono::system_clock::now()) & {
        if constexpr (sizeof...(Others) == 0) {
            return std::get<0>(*this).template run<Stage>(scheduler, time);
        } else {
            return [this, scheduler, time]<std::size_t... Indices>(
                       std::index_sequence<Indices...>) {
                return execution::when_all(
                    std::get<Indices>(*this).template run<Stage>(
                        scheduler, time)...);
            }(std::index_sequence_for<Env, Others...>{});
        }
    }

private:
    template <typename Alloc, std::size_t... Is>
    run_envs(std::index_sequence<Is...>, const Alloc& alloc = {})
        : std::tuple<Env, Others...>((static_cast<void>(Is), alloc)...) {}
};

template <typename Alloc, auto... Worlds>
using run_envs_for = type_list_rebind_t<
    run_envs, typename _run_envs_combine<typename _run_envs_for_impl<
                  Alloc, type_list<>, Worlds...>::type>::type>;

} // namespace neutron

namespace std {

template <std::size_t GroupId, typename... Worlds>
struct tuple_size<::neutron::run_env_for_group<GroupId, Worlds...>> {
    static constexpr std::size_t value = sizeof...(Worlds);
};

template <std::size_t Index, std::size_t GroupId, typename... Worlds>
struct tuple_element<Index, ::neutron::run_env_for_group<GroupId, Worlds...>> :
    ::neutron::type_list_element<Index, ::neutron::type_list<Worlds...>> {};

template <typename World>
struct tuple_size<::neutron::run_env_for_individual<World>> :
    integral_constant<size_t, 1> {};

template <typename World>
struct tuple_element<0, ::neutron::run_env_for_individual<World>> {
    using type = World;
};

template <typename... Envs>
struct tuple_size<::neutron::run_envs<Envs...>> {
    static constexpr std::size_t value = sizeof...(Envs);
};

template <std::size_t Index, typename... Envs>
struct tuple_element<Index, ::neutron::run_envs<Envs...>> :
    ::neutron::type_list_element<Index, ::neutron::type_list<Envs...>> {};

} // namespace std
