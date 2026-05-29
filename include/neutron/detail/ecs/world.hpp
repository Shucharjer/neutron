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
#include "neutron/detail/ecs/descriptor.hpp"
#include "neutron/detail/ecs/query.hpp"
#include "neutron/detail/ecs/world_base.hpp"
#include "neutron/detail/memory/rebind_alloc.hpp"
#include "neutron/memory.hpp"
#include "neutron/tuple.hpp"

namespace neutron {

template <typename Alloc>
class basic_world<world_descriptor_t<>, Alloc> : public world_base<Alloc> {
    template <auto, typename>
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
    using queries       = type_list<>;
    // using systems        = world_descriptor_t<>::systems;

    template <typename Al = Alloc>
    constexpr explicit basic_world(const Al& alloc = {})
        : world_base<Alloc>(alloc) {}

    template <stage Stage>
    static consteval auto get_tasks() noexcept {
        // return _world_task_set<Stage, descriptor_type>{};
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
};

template <typename Descriptor, typename Alloc = std::allocator<std::byte>>
class basic_world : public world_base<Alloc> {
    template <auto, typename>
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

    template <typename Al = Alloc>
    constexpr explicit basic_world(const Al& alloc = {})
        : world_base<Alloc>(alloc) /*, resources_(), locals_()*/ {}

    template <stage Stage>
    static consteval auto get_tasks() noexcept;

    void set_dynamic_update_interval(double seconds) noexcept {}

private:
    /// variables could be use in only one specific system
    /// Locals are _sys_tuple, a tuple with system info, used to get the correct
    /// local for each sys
    // type_list_rebind_t<neutron::shared_tuple, locals> locals_;
    // type_list_rebind_t<neutron::shared_tuple, queries> queries_;
    //  variables could be pass between each systems
    // type_list_rebind_t<neutron::shared_tuple, resources> resources_;
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
