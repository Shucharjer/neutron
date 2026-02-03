#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include "neutron/detail/ecs/archetype.hpp"
#include "neutron/detail/ecs/command_buffer.hpp"
#include "neutron/detail/ecs/metainfo.hpp"
#include "neutron/detail/ecs/world_base.hpp"
#include "neutron/detail/ecs/world_descriptor.hpp"
#include "neutron/detail/memory/rebind_alloc.hpp"
#include "neutron/execution.hpp"
#include "neutron/memory.hpp"
#include "neutron/tuple.hpp"

namespace neutron {

namespace _basic_world {} // namespace _basic_world

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
    using allocator_type = Alloc;
    using command_buffer = ::neutron::command_buffer<Alloc>;
    using query_type     = basic_querior<Alloc>;
    using commands_type  = basic_commands<Alloc>;
    using sysinfo        = world_descriptor_t<>::sysinfo;
    // using systems        = world_descriptor_t<>::systems;

    template <typename Al = Alloc>
    constexpr explicit basic_world(const Al& alloc = {})
        : world_base<Alloc>(alloc) {}

private:
    _vector_t<command_buffer>* command_buffers_;
};

template <typename Descriptor, std_simple_allocator Alloc>
class basic_world : public world_base<Alloc> {
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
    using allocator_type = Alloc;
    using archetype      = ::neutron::archetype<_byte_alloc>;
    using command_buffer = ::neutron::command_buffer<_byte_alloc>;

    using desc_traits = descriptor_traits<Descriptor>;
    using sysinfo     = typename desc_traits::sysinfo;
    using grouped     = typename desc_traits::grouped;
    using run_lists   = typename desc_traits::runlists;
    using locals      = typename desc_traits::locals;
    using resources   = typename desc_traits::resources;

    template <typename Al = Alloc>
    constexpr explicit basic_world(const Al& alloc = {})
        : world_base<Alloc>(alloc) /*, resources_(), locals_()*/ {}

    template <stage Stage, neutron::execution::scheduler Sch>
    void call(Sch& sch, _vector_t<command_buffer>& cmdbufs) {
        using run_list = _fetch_run_list<Stage, typename run_lists::all>::type;
        command_buffers_ = &cmdbufs;
        _call_run_list<run_list>{}(sch, this);
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

    template <stage Stage, typename RunLists>
    struct _fetch_run_list {
        template <typename RunList>
        using _is_relevant_run_list = is_staged_type_list<Stage, RunList>;

        using type = typename type_list_first_t<
            type_list_filt_t<_is_relevant_run_list, RunLists>>::type;
    };

    template <typename RunList>
    struct _call_run_list;
    template <typename... SystemLists>
    struct _call_run_list<type_list<SystemLists...>> {
        template <typename SystemList>
        struct _call_system_list;
        template <typename... SysInfo>
        struct _call_system_list<type_list<SysInfo...>> {
            template <size_t Index, auto Sys, typename T = decltype(Sys)>
            struct _call_sys;
            template <size_t Index, auto Sys, typename Ret, typename... Args>
            struct _call_sys<Index, Sys, Ret (*)(Args...)> {
                void operator()(basic_world* world) const {
                    Sys(construct_from_world<Sys, Args, Index>(*world)...);
                }
            };
            template <size_t Index, auto Sys, typename Ret, typename... Args>
            struct _call_sys<Index, Sys, Ret (*)(Args...) noexcept> {
                void operator()(basic_world* world) const noexcept {
                    Sys(construct_from_world<Sys, Args, Index>(*world)...);
                }
            };

            template <execution::scheduler Sch>
            void operator()(Sch& sch, basic_world* world) const {
                using namespace execution;
                for (command_buffer& cmdbuf : *world->command_buffers_) {
                    cmdbuf.reset();
                }

                auto all = [&sch,
                            world]<size_t... Is>(std::index_sequence<Is...>) {
                    return when_all((schedule(sch) | then([world] {
                                         _call_sys<Is, SysInfo::fn>{}(world);
                                     }))...);
                }(std::index_sequence_for<SysInfo...>());
                sync_wait(std::move(all));
                world->_apply_command_buffers();
            }
        };

        template <execution::scheduler Sch>
        void operator()(Sch& sch, basic_world* world) {
            [&sch, world]<size_t... Is>(std::index_sequence<Is...>) {
                (_call_system_list<SystemLists>{}(sch, world), ...);
            }(std::index_sequence_for<SystemLists...>());
        }
    };

    void _apply_command_buffers() noexcept {
        for (auto& cmdbuf : *command_buffers_) {
            cmdbuf.apply(_base());
        }
    }

    /// variables could be use in only one specific system
    /// Locals are _sys_tuple, a tuple with system info, used to get the correct
    /// local for each sys
    type_list_rebind_t<neutron::shared_tuple, locals> locals_;
    //  variables could be pass between each systems
    type_list_rebind_t<neutron::shared_tuple, resources> resources_;

    _vector_t<command_buffer>* command_buffers_;
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

template <
    stage Stage, neutron::execution::scheduler Sch, world World,
    typename CmdBuf                     = World::command_buffer,
    template <typename> typename Vector = World::template _vector_t>
void call(Sch& sch, Vector<CmdBuf>& cmdbufs, World& world) {
    world.template call<Stage>(sch, cmdbufs);
}

template <
    stage Stage, neutron::execution::scheduler Sch, typename Alloc,
    world... Worlds,
    typename VectorAlloc =
        neutron::rebind_alloc_t<Alloc, command_buffer<Alloc>>>
void call(
    Sch& sch, std::vector<command_buffer<Alloc>, VectorAlloc>& cmdbufs,
    std::tuple<Worlds...>& worlds) {
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        (std::get<Is>(worlds).template call<Stage>(sch, cmdbufs), ...);
    }(std::index_sequence_for<Worlds...>());
}

void call_startup(
    neutron::execution::scheduler auto& sch, auto& cmdbufs, world auto& world) {
    call<stage::pre_startup>(sch, cmdbufs, world);
    call<stage::startup>(sch, cmdbufs, world);
    call<stage::post_startup>(sch, cmdbufs, world);
}

template <world... Worlds>
void call_startup(
    neutron::execution::scheduler auto& sch, auto& cmdbufs,
    std::tuple<Worlds...>& worlds) {
    call<stage::pre_startup>(sch, cmdbufs, worlds);
    call<stage::startup>(sch, cmdbufs, worlds);
    call<stage::post_startup>(sch, cmdbufs, worlds);
}

void call_update(
    neutron::execution::scheduler auto& sch, auto& cmdbufs, world auto& world) {
    call<stage::pre_update>(sch, cmdbufs, world);
    call<stage::update>(sch, cmdbufs, world);
    call<stage::post_update>(sch, cmdbufs, world);
}

template <world... Worlds>
void call_update(
    neutron::execution::scheduler auto& sch, auto& cmdbufs,
    std::tuple<Worlds...>& worlds) {
    call<stage::pre_update>(sch, cmdbufs, worlds);
    call<stage::update>(sch, cmdbufs, worlds);
    call<stage::post_update>(sch, cmdbufs, worlds);
}

} // namespace neutron
