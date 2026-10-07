// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <vector>
#include "neutron/detail/ecs/compile-time/collect_params/local.hpp"
#include "neutron/detail/ecs/compile-time/collect_params/query.hpp"
#include "neutron/detail/ecs/compile-time/collect_params/res.hpp"
#include "neutron/detail/ecs/compile-time/descriptor.hpp"
#include "neutron/detail/ecs/compile-time/queries.hpp"
#include "neutron/detail/ecs/core/archetype.hpp"
#include "neutron/detail/ecs/core/command_buffer.hpp"
#include "neutron/detail/ecs/core/world_base.hpp"
#include "neutron/detail/ecs/runtime/tasks.hpp"
#include "neutron/detail/ecs/utility/byte_allocator.hpp"
#include "neutron/detail/memory/rebind_alloc.hpp"
#include "neutron/detail/tuple/shared_tuple.hpp"
#include "neutron/memory.hpp"
#include "neutron/metafn.hpp"
#include "neutron/tuple.hpp"

namespace neutron {

class _tick_rate_base {
public:
    void set_last_update(_time_point_t time) noexcept { _last_update = time; }
    ATOM_NODISCARD _time_point_t get_last_update() const noexcept {
        return _last_update;
    }

protected:
    _time_point_t _last_update{};
};

template <typename Descriptor>
class _basic_world_tick_rate_t : public _tick_rate_base {
public:
    static constexpr auto get_tick_interval() noexcept {
        constexpr auto rate = get_forward_tick_rate(Descriptor());
        return std::chrono::duration<double>{ rate == 0.0 ? 0.0 : 1.0 / rate };
    }

    ATOM_NODISCARD bool should_update(_time_point_t time) const noexcept {
        using namespace std::chrono;
        constexpr double forward_tick_rate =
            get_forward_tick_rate(Descriptor());
        if constexpr (forward_tick_rate == 0.0) {
            return true;
        } else {
            const auto dur = duration<double>(1 / forward_tick_rate);
            return time >= _last_update + dur;
        }
    }
};
template <typename Descriptor>
requires(get_forward_tick_rate(Descriptor()) < 0.0)
class _basic_world_tick_rate_t<Descriptor> : public _tick_rate_base {
public:
    ATOM_NODISCARD bool should_update(_time_point_t time) const noexcept {
        using namespace std::chrono;
        return time >= _last_update + interval_;
    }
    ATOM_NODISCARD int get_tick_rate() const noexcept { return rate_; }
    ATOM_NODISCARD auto get_tick_interval() const noexcept { return interval_; }
    void set_tick_rate(int tick_rate) {
        if (tick_rate < 0) {
            throw std::invalid_argument("tick rate must be nonnegative");
        }
        rate_     = tick_rate;
        interval_ = std::chrono::duration<double>(
            tick_rate == 0 ? 0.0 : 1.0 / tick_rate);
    }

private:
    int rate_ = 120;
    std::chrono::duration<double> interval_{ 1.0 / 120.0 }; // NOLINT
};

template <typename Descriptor>
class _basic_world_task_base : public _basic_world_tick_rate_t<Descriptor> {
protected:
    ATOM_NO_UNIQUE_ADDR internal::world_tasks<Descriptor> tasks_;
};

template <typename Desc, typename Alloc = std::allocator<std::byte>>
class basic_world;

template <typename Alloc>
constexpr bool _is_byte_allocator =
    std::same_as<typename std::allocator_traits<Alloc>::value_type, std::byte>;

template <typename Descriptor, typename Alloc>
requires internal::byte_allocator<Alloc>
class basic_world<Descriptor, Alloc> :
    public _basic_world_task_base<Descriptor>,
    public world_base<Alloc> {
    template <stage Stage, auto, typename>
    friend struct construct_from_world_t;
    friend struct world_accessor;

    template <typename TypeList>
    using _shared_tuple = type_list_rebind_t<shared_tuple, TypeList>;

    auto _base() & noexcept -> world_base<Alloc>& {
        return *static_cast<world_base<Alloc>*>(this);
    }

    auto _base() const& noexcept -> const world_base<Alloc>& {
        return *static_cast<const world_base<Alloc>*>(this);
    }

    template <typename Ty>
    using _allocator_t = rebind_alloc_t<Alloc, Ty>;

    template <typename Ty>
    using _vector_t = ::std::vector<Ty, _allocator_t<Ty>>;

public:
    using descriptor_type = Descriptor;
    using allocator_type  = Alloc;
    using archetype       = ::neutron::archetype<Alloc>;
    using command_buffer  = ::neutron::command_buffer<Alloc>;

    static constexpr bool individual =
        get_execution_policy(Descriptor()).is_individual;
    static constexpr std::size_t group_id =
        get_execution_policy(Descriptor()).id;

    template <typename Al = Alloc>
    constexpr explicit basic_world(const Al& alloc = {})
        : world_base<Alloc>(alloc) /*, resources_(), locals_()*/ {}

    template <stage Stage>
    static consteval auto get_tasks() noexcept {
        return get_systems<Stage>(Descriptor{});
    }

    template <stage Stage, execution::scheduler Scheduler>
    ATOM_NODISCARD execution::sender auto
        run(Scheduler scheduler,
            _time_point_t time = std::chrono::system_clock::now()) & {
        auto& tasks = std::get<static_cast<std::size_t>(Stage)>(this->tasks_);
        if constexpr (
            std::remove_reference_t<decltype(tasks)>::plan::count == 0) {
            return execution::just();
        } else {
            auto sender = tasks.run(*this, scheduler, time);
            if constexpr (
                is_hifreq_stage(Stage) &&
                get_forward_tick_rate(Descriptor{}) != 0.0) {
                return internal::conditional_sender{
                    tasks.clock.due(time, this->get_tick_interval()),
                    std::move(sender) | execution::then([&tasks, time] {
                        tasks.clock.complete(time);
                    })
                };
            } else {
                return sender;
            }
        }
    }

    template <stage Stage, auto Function>
    void set_system_tick_rate(int rate) {
        using plan = internal::task_plan<decltype(get_tasks<Stage>())>;
        constexpr auto index =
            []<std::size_t... Indices>(std::index_sequence<Indices...>) {
                std::size_t result = plan::count;
                ((_task_spec_matches<
                      value_list_element_v<Indices, typename plan::specs>,
                      Function>()
                      ? result = Indices
                      : result),
                 ...);
                return result;
            }(std::make_index_sequence<plan::count>{});
        static_assert(
            index < plan::count, "system was not added to this stage");
        if constexpr (index < plan::count) {
            static_assert(
                is_hifreq_stage(Stage) && plan::tick_rates[index] < 0,
                "system requires dynamic_tick_rate in a high-frequency stage");
            auto& tasks =
                std::get<static_cast<std::size_t>(Stage)>(this->tasks_);
            std::get<index>(tasks.clocks).set_tick_rate(rate);
        }
    }

private:
    using resources_type = unique_type_list_t<type_list_convert_t<
        std::remove_cvref,
        type_list_export_as_t<
            res, collect_params_of<descriptor_type, res, false>>>>;
    using locals_type =
        collect_params_with_sys_of<descriptor_type, local, false>;

    ATOM_NO_UNIQUE_ADDR _shared_tuple<query_cache_of<descriptor_type>> queries_;
    ATOM_NO_UNIQUE_ADDR _shared_tuple<resources_type> resources_;
    ATOM_NO_UNIQUE_ADDR _shared_tuple<locals_type> locals_;
};

template <typename Desc, typename Alloc>
class basic_world : public basic_world<Desc, rebind_alloc_t<Alloc, std::byte>> {
public:
    using basic_world<Desc, rebind_alloc_t<Alloc, std::byte>>::basic_world;
};

template <auto Desc, typename Alloc = std::allocator<std::byte>>
using world_of = basic_world<std::remove_cvref_t<decltype(Desc)>, Alloc>;

template <
    descriptor auto Descriptor, typename Alloc = std::allocator<std::byte>>
auto make_world(const Alloc& alloc = {})
    -> basic_world<decltype(Descriptor), Alloc> {
    return basic_world<decltype(Descriptor), Alloc>(alloc);
}

template <
    descriptor auto... Descriptors, typename Alloc = std::allocator<std::byte>>
auto make_worlds(const Alloc& alloc = {}) -> std::tuple<
    basic_world<decltype(Descriptors), rebind_alloc_t<Alloc, std::byte>>...> {
    return {
        basic_world<decltype(Descriptors), rebind_alloc_t<Alloc, std::byte>>(
            alloc)...
    };
}

} // namespace neutron
