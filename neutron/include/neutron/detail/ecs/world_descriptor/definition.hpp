// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/stage.hpp"
#include "neutron/detail/ecs/world_descriptor/add_system.hpp"
#include "neutron/detail/ecs/world_descriptor/fwd.hpp"
#include "neutron/detail/metafn/cat.hpp"
#include "neutron/detail/metafn/definition.hpp"

namespace neutron {

template <typename SysInfo>
struct world_descriptor_t {
    template <typename... Tparams>
    using type_list_cat_t = neutron::type_list_cat_t<Tparams...>;
    template <typename... Tparams>
    using value_list_cat_t = neutron::value_list_cat_t<Tparams...>;

    using sysinfo = SysInfo;

    template <stage Stage, auto Fn, typename... Requires>
    using add_system_t = world_descriptor_t<type_list_cat_t<
        SysInfo, type_list<_add_system::sysinfo<Stage, Fn, Requires...>>>>;
};

constexpr inline world_descriptor_t<> world_desc;

} // namespace neutron
