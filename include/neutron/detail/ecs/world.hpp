// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>
#include "neutron/detail/ecs/archetype.hpp"
#include "neutron/detail/ecs/command_buffer.hpp"
#include "neutron/detail/ecs/metainfo.hpp"
#include "neutron/detail/ecs/query.hpp"
#include "neutron/detail/ecs/task_graph.hpp"
#include "neutron/detail/ecs/world_base.hpp"
#include "neutron/detail/ecs/world_descriptor.hpp"
#include "neutron/detail/memory/rebind_alloc.hpp"
#include "neutron/memory.hpp"
#include "neutron/tuple.hpp"

namespace neutron {

namespace _basic_world {

template <typename Descriptor, bool = world_descriptor<Descriptor>>
struct _descriptor_validator;

template <typename Descriptor>
struct _descriptor_validator<Descriptor, true> {
    static_assert(
        descriptor_traits<Descriptor>::value, "Invalid ECS world descriptor");
};

template <typename Descriptor>
struct _descriptor_validator<Descriptor, false> {
    static_assert(
        world_descriptor<Descriptor>,
        "basic_world requires a world descriptor");
};

} // namespace _basic_world

template <std_simple_allocator Alloc>
class basic_world<world_descriptor_t<>, Alloc> : public world_base<Alloc> {
    template <auto, typename, size_t>
    friend struct construct_from_world_t;
    friend struct world_accessor;

    template <typename Ty>
    using _allocator_t = rebind_alloc_t<Alloc, Ty>;

    template <typename Ty>
    using _vector_t = std::vector<Ty, _allocator_t<Ty>>;

public:
    using descriptor_type = world_descriptor_t<>;
    using allocator_type  = Alloc;
    using command_buffer  = ::neutron::command_buffer<Alloc>;
    template <typename... Filters>
    using query_type    = query<Filters...>;
    using commands_type = basic_commands<Alloc>;
    using sysinfo       = sysinfo_holder<world_descriptor_t<>>;
    using queries       = type_list<>;
    // using systems        = world_descriptor_t<>::systems;

    template <typename Al = Alloc>
    constexpr explicit basic_world(const Al& alloc = {})
        : world_base<Alloc>(alloc) {}

    template <stage Stage>
    static consteval auto get_tasks() noexcept {
        return _world_task_set<Stage, descriptor_type>{};
    }

    void set_dynamic_update_interval(double) noexcept {}

    [[nodiscard]] constexpr double dynamic_update_interval() const noexcept {
        return 0.0;
    }

    [[nodiscard]] constexpr bool has_dynamic_update_interval() const noexcept {
        return false;
    }

    [[nodiscard]] constexpr bool should_call_update() noexcept { return true; }

private:
    type_list_rebind_t<neutron::shared_tuple, queries> queries_{};
    const insertion_context* insertion_context_ = nullptr;
};

template <typename Descriptor, std_simple_allocator Alloc>
class basic_world :
    private _basic_world::_descriptor_validator<Descriptor>,
    public world_base<Alloc> {
    template <auto, typename, size_t>
    friend struct construct_from_world_t;
    friend struct world_accessor;

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

    using desc_traits    = descriptor_traits<Descriptor>;
    using execute_traits = execute_info<Descriptor>;
    using sysinfo        = typename desc_traits::sysinfo;
    using grouped        = typename desc_traits::grouped;
    using run_lists      = typename desc_traits::runlists;
    using components     = typename desc_traits::components;
    using locals         = typename desc_traits::locals;
    using queries        = typename desc_traits::queries;
    using resources      = typename desc_traits::resources;
    using clock_type     = std::chrono::steady_clock;

    template <typename Al = Alloc>
    constexpr explicit basic_world(const Al& alloc = {})
        : world_base<Alloc>(alloc) /*, resources_(), locals_()*/ {}

    template <stage Stage>
    static consteval auto get_tasks() noexcept {
        return _world_task_set<Stage, descriptor_type>{};
    }

    void set_dynamic_update_interval(double seconds) noexcept {
        if (seconds > 0.0) {
            dynamic_update_interval_     = seconds;
            has_dynamic_update_interval_ = true;
        } else {
            dynamic_update_interval_     = 0.0;
            has_dynamic_update_interval_ = false;
        }
    }

    [[nodiscard]] double dynamic_update_interval() const noexcept {
        return dynamic_update_interval_;
    }

    [[nodiscard]] bool has_dynamic_update_interval() const noexcept {
        return has_dynamic_update_interval_;
    }

    [[nodiscard]] bool should_call_update() noexcept {
        if constexpr (execute_traits::is_always) {
            return true;
        }

        const auto interval = _update_interval();
        if (interval <= 0.0) {
            return false;
        }

        const auto now = clock_type::now();
        if (!has_last_update_ ||
            std::chrono::duration<double>(now - last_update_).count() >=
                interval) {
            last_update_     = now;
            has_last_update_ = true;
            return true;
        }
        return false;
    }

private:
    // static constexpr auto _hash_array() noexcept {
    //     return neutron::make_hash_array<components>();
    // }

    // template <component Component>
    // static consteval index_t _static_index() {
    //     static_assert(std::same_as<Component,
    //     std::remove_cvref_t<Component>>);

    //     constexpr auto array = _hash_array();
    //     constexpr auto hash  = neutron::hash_of<Component>();
    //     auto it = std::lower_bound(array.begin(), array.end(), hash);
    //     if (it != array.end()) {
    //         return *it;
    //     }
    //     throw std::invalid_argument("Component not exists");
    // }

    [[nodiscard]] double _update_interval() const noexcept {
        if constexpr (execute_traits::is_always) {
            return 0.0;
        } else if constexpr (execute_traits::has_dynamic_interval) {
            return has_dynamic_update_interval_ ? dynamic_update_interval_
                                                : 0.0;
        } else if constexpr (execute_traits::has_interval) {
            return execute_traits::execution_interval;
        } else {
            return 0.0;
        }
    }

    /// variables could be use in only one specific system
    /// Locals are _sys_tuple, a tuple with system info, used to get the correct
    /// local for each sys
    type_list_rebind_t<neutron::shared_tuple, locals> locals_;
    type_list_rebind_t<neutron::shared_tuple, queries> queries_;
    //  variables could be pass between each systems
    type_list_rebind_t<neutron::shared_tuple, resources> resources_;

    const insertion_context* insertion_context_ = nullptr;
    typename clock_type::time_point last_update_{};
    double dynamic_update_interval_   = 0.0;
    bool has_dynamic_update_interval_ = false;
    bool has_last_update_             = false;
};

template <
    world_descriptor auto Descriptor,
    typename Alloc = std::allocator<std::byte>>
auto make_world(const Alloc& alloc = {})
    -> basic_world<decltype(Descriptor), Alloc> {
    return basic_world<decltype(Descriptor), Alloc>(alloc);
}

template <
    world_descriptor auto... Descriptors,
    typename Alloc = std::allocator<std::byte>>
auto make_worlds(const Alloc& alloc = {}) -> std::tuple<
    basic_world<decltype(Descriptors), rebind_alloc_t<Alloc, std::byte>>...> {
    return {
        basic_world<decltype(Descriptors), rebind_alloc_t<Alloc, std::byte>>(
            alloc)...
    };
}

} // namespace neutron
