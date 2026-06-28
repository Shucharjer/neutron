// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <vector>
#include "neutron/detail/ecs/compile-time/collect_params/local.hpp"
#include "neutron/detail/ecs/compile-time/collect_params/query.hpp"
#include "neutron/detail/ecs/compile-time/collect_params/res.hpp"
#include "neutron/detail/ecs/compile-time/descriptor.hpp"
#include "neutron/detail/ecs/compile-time/queries.hpp"
#include "neutron/detail/ecs/core/archetype.hpp"
#include "neutron/detail/ecs/core/command_buffer.hpp"
#include "neutron/detail/ecs/core/world_base.hpp"
#include "neutron/detail/memory/rebind_alloc.hpp"
#include "neutron/detail/tuple/shared_tuple.hpp"
#include "neutron/memory.hpp"
#include "neutron/metafn.hpp"
#include "neutron/tuple.hpp"

namespace neutron {

using time_point_t = std::chrono::system_clock::time_point;

class _interval_base {
public:
    void set_last_update(time_point_t time) noexcept { _last_update = time; }
    ATOM_NODISCARD time_point_t get_last_update() const noexcept {
        return _last_update;
    }

protected:
    time_point_t _last_update;
};

template <typename Descriptor>
class _basic_world_interval_t : public _interval_base {
public:
    ATOM_NODISCARD bool should_update(time_point_t time) const noexcept {
        using namespace std::chrono;
        constexpr double forward_interval = get_forward_interval(Descriptor());
        if constexpr (forward_interval == 0.0) {
            return true;
        } else {
            return time >= _last_update + duration<double>(forward_interval);
        }
    }
};
template <typename Descriptor>
requires(get_forward_interval(Descriptor()) < 0.0)
class _basic_world_interval_t<Descriptor> : public _interval_base {
public:
    ATOM_NODISCARD bool should_update(time_point_t time) const noexcept {
        using namespace std::chrono;
        return time >= _last_update + duration<double>(interval_);
    }
    ATOM_NODISCARD double get_interval() const noexcept { return interval_; }
    void set_interval(double interval) noexcept { this->interval_ = interval; }

private:
    double interval_ = 1.0 / 120.0; // NOLINT
};

template <typename Descriptor>
class _basic_world_task_base : public _basic_world_interval_t<Descriptor> {
public:
    template <stage Stage>
    auto get_tasks() const noexcept
    /* the result of get_tasks should be a inplace_vector */ {
        //
    }
};

template <typename Descriptor>
requires(get_forward_interval(Descriptor()) == 0.0)
class _basic_world_task_base<Descriptor> :
    public _basic_world_interval_t<Descriptor> {
public:
    template <stage Stage>
    constexpr auto get_tasks() const noexcept
    /* the result of get_tasks should be a inplace_vector */ {
        // return get_systems<Stage>(Descriptor());
    }
};

template <typename Descriptor, typename Alloc = std::allocator<std::byte>>
class basic_world :
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
    using _byte_alloc  = _allocator_t<::std::byte>;

    template <typename Ty>
    using _vector_t = ::std::vector<Ty, _allocator_t<Ty>>;

public:
    using descriptor_type = Descriptor;
    using allocator_type  = Alloc;
    using archetype       = ::neutron::archetype<_byte_alloc>;
    using command_buffer  = ::neutron::command_buffer<_byte_alloc>;

    template <typename Al = Alloc>
    constexpr explicit basic_world(const Al& alloc = {})
        : world_base<Alloc>(alloc) /*, resources_(), locals_()*/ {}

    template <stage Stage>
    static consteval auto get_tasks() noexcept;

private:
    _shared_tuple<query_cache_of<descriptor_type>> queries_;
    _shared_tuple<res_of<descriptor_type>> resources_;
    _shared_tuple<local_of<descriptor_type>> locals_;
};

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
