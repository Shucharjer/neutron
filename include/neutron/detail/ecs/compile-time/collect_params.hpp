#pragma once
#include <type_traits>
#include <neutron/concepts.hpp>
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/compile-time/descriptor.hpp"
#include "neutron/detail/ecs/compile-time/queries.hpp"

#include "neutron/detail/ecs/params/local.hpp"
#include "neutron/detail/ecs/params/res.hpp"

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */
namespace _collect_params {

using enum stage;

template <stage Stage, descriptor Desc, template <typename...> typename Tmp>
struct _collect_impl {
    template <typename T>
    using _is_tmp = _is_instance_of_impl<T, Tmp>;

    template <auto SysSpec>
    struct _to_param_list {
        using type =
            type_list_filt_t<_is_tmp, typename decltype(SysSpec)::fn_args>;
    };

    using spec_list  = decltype(get_systems<Stage>(Desc()))::spec_list;
    using param_list = type_list_from_value_t<_to_param_list, spec_list>;
    using type       = type_list_export_as_t<type_list, param_list>;
};

template <descriptor Desc, template <typename...> typename Tmp, bool HiFreq>
struct _collect {
    using prestartup_t  = _collect_impl<prestartup, Desc, Tmp>::type;
    using startup_t     = _collect_impl<startup, Desc, Tmp>::type;
    using poststartup_t = _collect_impl<poststartup, Desc, Tmp>::type;
    using first_t       = _collect_impl<first, Desc, Tmp>::type;
    using last_t        = _collect_impl<last, Desc, Tmp>::type;
    using shutdown_t    = _collect_impl<shutdown, Desc, Tmp>::type;

    using events_t     = _collect_impl<events, Desc, Tmp>::type;
    using preupdate_t  = _collect_impl<preupdate, Desc, Tmp>::type;
    using update_t     = _collect_impl<update, Desc, Tmp>::type;
    using postupdate_t = _collect_impl<postupdate, Desc, Tmp>::type;
    using render_t     = _collect_impl<render, Desc, Tmp>::type;

    // clang-format off
    using lowfreq = type_list_cat_t<prestartup_t, startup_t, poststartup_t, first_t, last_t, shutdown_t>;
    using hifreq  = type_list_cat_t<events_t, preupdate_t, update_t, postupdate_t, render_t>;
    // clang-format on

    using type =
        std::conditional_t<HiFreq, hifreq, type_list_cat_t<lowfreq, hifreq>>;
};

template <stage Stage, descriptor Desc, template <typename...> typename Tmp>
struct _collect_with_sys_impl {
    template <typename T>
    using _is_tmp = _is_instance_of_impl<T, Tmp>;

    template <auto SysSpec>
    struct _to_param_list {
        using param_list =
            type_list_filt_t<_is_tmp, typename decltype(SysSpec)::fn_args>;

        template <typename T>
        struct _to_systuple;

        template <typename... Args>
        struct _to_systuple<Tmp<Args...>> {
            using type = systuple<Stage, SysSpec.fn, Args...>;
        };

        using type = type_list_convert_t<_to_systuple, param_list>;
    };

    using spec_list  = decltype(get_systems<Stage>(Desc()))::spec_list;
    using param_list = type_list_from_value_t<_to_param_list, spec_list>;
    using type       = type_list_export_as_t<type_list, param_list>;
};

template <descriptor Desc, template <typename...> typename Tmp, bool HiFreq>
struct _collect_with_sys {
    using prestartup_t  = _collect_with_sys_impl<prestartup, Desc, Tmp>::type;
    using startup_t     = _collect_with_sys_impl<startup, Desc, Tmp>::type;
    using poststartup_t = _collect_with_sys_impl<poststartup, Desc, Tmp>::type;
    using first_t       = _collect_with_sys_impl<first, Desc, Tmp>::type;
    using last_t        = _collect_with_sys_impl<last, Desc, Tmp>::type;
    using shutdown_t    = _collect_with_sys_impl<shutdown, Desc, Tmp>::type;

    using events_t     = _collect_with_sys_impl<events, Desc, Tmp>::type;
    using preupdate_t  = _collect_with_sys_impl<preupdate, Desc, Tmp>::type;
    using update_t     = _collect_with_sys_impl<update, Desc, Tmp>::type;
    using postupdate_t = _collect_with_sys_impl<postupdate, Desc, Tmp>::type;
    using render_t     = _collect_with_sys_impl<render, Desc, Tmp>::type;

    // clang-format off
    using lowfreq = type_list_cat_t<prestartup_t, startup_t, poststartup_t, first_t, last_t, shutdown_t>;
    using hifreq  = type_list_cat_t<events_t, preupdate_t, update_t, postupdate_t, render_t>;
    // clang-format on

    using type =
        std::conditional_t<HiFreq, hifreq, type_list_cat_t<lowfreq, hifreq>>;
};

} // namespace _collect_params
/*! @endcond */

template <descriptor Desc, template <typename...> typename Tmp, bool HiFreq>
using collect_params_of =
    typename _collect_params::_collect<Desc, Tmp, HiFreq>::type;

template <descriptor Desc, template <typename...> typename Tmp, bool HiFreq>
using collect_params_with_sys_of =
    typename _collect_params::_collect_with_sys<Desc, Tmp, HiFreq>::type;

} // namespace neutron
