// IWYU pragma: private, include "neutron/detail/ecs/world_descriptor.hpp"
#pragma once
#include <neutron/detail/metafn/empty.hpp>
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/stage.hpp"
#include "neutron/detail/ecs/world_descriptor/fwd.hpp"

namespace neutron {

template <typename Ty>
constexpr bool as_system_requirement = false;

namespace _add_system {

template <typename Ty>
concept system_requirement = (requires {
    typename std::remove_cvref_t<Ty>::system_requirement_concept;
    requires std::derived_from<
        typename std::remove_cvref_t<Ty>::system_requirement_concept,
        system_requirement_t>;
} || as_system_requirement<std::remove_cvref_t<Ty>>);

template <auto Sys>
struct _before_t {
    using system_requirement_concept = system_requirement_t;
};

template <auto Sys>
struct _after_t {
    using system_requirement_concept = system_requirement_t;
};

// e.g. { &foo, after<&bar> }
template <typename Sys, auto... Requires>
struct sysdesc {
    Sys fn;
    constexpr sysdesc(Sys fn, decltype(Requires)...) noexcept : fn(fn) {}
};

template <typename Fn, system_requirement... Requires>
sysdesc(Fn, Requires...) -> sysdesc<Fn, Requires{}...>;

template <stage Stage>
struct sys_tag_t {};

template <stage Stage, sysdesc... Systems>
struct _add_systems_t :
    public descriptor_adaptor_closure<_add_systems_t<Stage, Systems...>> {
    template <world_descriptor Descriptor>
    consteval auto operator()(Descriptor descriptor) const noexcept {
        return insert_range_tagged_value_list_inplace_t<
            sys_tag_t<Stage>, Descriptor, tagged_value_list, Systems...>{};
    }
};

} // namespace _add_system

using _add_system::sysdesc;
using _add_system::sys_tag_t;

template <stage Stage, sysdesc... Systems>
inline constexpr _add_system::_add_systems_t<Stage, Systems...> add_systems;

template <auto Sys>
inline constexpr _add_system::_before_t<Sys> before;

template <auto Sys>
inline constexpr _add_system::_after_t<Sys> after;

} // namespace neutron

#if ATOM_HAS_REFLECTION
namespace neutron {

template <stage Stage, sysdesc... Desc>
struct _add_systems_t {
    static constexpr auto infos = std::define_static_array({(^^Desc)...});
};

template <stage Stage, sysdesc... Desc>
constexpr auto _add_systems_t<Stage, Desc...> add_systems;

add_systems<update, { fn }, { afn, after<fn> }>;

} // namespace neutron
#endif
