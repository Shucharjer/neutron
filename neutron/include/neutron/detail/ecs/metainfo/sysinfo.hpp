#pragma once
#include "neutron/detail/ecs/metainfo/common.hpp"
#include "neutron/detail/ecs/world_descriptor/add_systems.hpp"

namespace neutron {

namespace _metainfo {

using enum stage;

template <stage Stage, typename TypeList>
struct fetch_sysinfo {
    using type = fetch_vinfo_t<sys_tag_t<Stage>, TypeList>;
};
template <stage Stage, typename TypeList>
using fetch_sysinfo_t = typename fetch_sysinfo<Stage, TypeList>::type;

template <stage Stage, typename TypeList>
using fetch_sysinfo_t = typename fetch_sysinfo<Stage, TypeList>::type;

template <typename Descriptor>
struct sysinfo_holder {
    // clang-format off
    using prestartup_info  = fetch_sysinfo_t<pre_startup, Descriptor>;
    using startup_info     = fetch_sysinfo_t<startup, Descriptor>;
    using poststartup_info = fetch_sysinfo_t<post_startup, Descriptor>;

    using first_info       = fetch_sysinfo_t<first, Descriptor>;

    using preupdate_info   = fetch_sysinfo_t<pre_update, Descriptor>;
    using update_info      = fetch_sysinfo_t<update, Descriptor>;
    using postupdate_info  = fetch_sysinfo_t<post_update, Descriptor>;

    using render_info      = fetch_sysinfo_t<render, Descriptor>;

    using shutdown_info    = fetch_sysinfo_t<shutdown, Descriptor>;

    using last_info        = fetch_sysinfo_t<last, Descriptor>;
    // clang-format on
};

template <auto Fn, typename = decltype(Fn)>
struct _fn_traits;
template <auto Fn, typename Ret, typename... Args>
struct _fn_traits<Fn, Ret (*)(Args...)> {
    using arg_list = type_list<Args...>;

    using resource = type_list_first_t<
        type_list_filt_nempty_t<internal::_is_res, arg_list, type_list<res<>>>>;

    template <typename... Tys>
    using _systuple = systuple<Fn, Tys...>;
    using local     = type_list_rebind_t<
            _systuple,
            type_list_first_t<type_list_filt_nempty_t<
                internal::_is_local, arg_list, type_list<::neutron::local<>>>>>;
};

} // namespace _metainfo

using _metainfo::sysinfo_holder;

} // namespace neutron
