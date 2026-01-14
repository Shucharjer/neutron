#pragma once
#include <type_traits>
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/stage.hpp"
#include "neutron/detail/ecs/world_descriptor.hpp"
#include "neutron/detail/ecs/world_descriptor/fwd.hpp"
#include "neutron/detail/metafn/definition.hpp"
#include "neutron/detail/metafn/filt.hpp"
#include "neutron/detail/metafn/sort.hpp"

namespace neutron {

namespace _descriptor_traits {

template <typename SysInfo>
struct _group_by_stage {
    // maybe unuseful
    // type_list<sysinfo<...>, ...>
    using sortedlist = type_list_sort_t<_add_system::sysinfo_cmp_less, SysInfo>;

    template <stage Stage, typename TypeList>
    struct _filt_by_stage {
        template <typename Info>
        using predicate_type = std::bool_constant<Stage == Info::stage>;
        using type_list      = type_list_filt_t<predicate_type, TypeList>;
    };

    template <stage Stage, typename Filted>
    struct _staged;
    template <stage Stage, typename... Info>
    struct _staged<Stage, type_list<Info...>> {
        using type = staged_type_list<Stage, Info...>;
    };

    // Build a staged_type_list from the filtered sysinfo type_list for Stage
    template <stage Stage, typename TypeList>
    using _staged_list = typename _staged<
        Stage, typename _filt_by_stage<Stage, TypeList>::type_list>::type;

    using prestartups  = _staged_list<stage::pre_startup, sortedlist>;
    using startups     = _staged_list<stage::startup, sortedlist>;
    using poststartups = _staged_list<stage::post_startup, sortedlist>;
    using firsts       = _staged_list<stage::first, sortedlist>;
    using preupdates   = _staged_list<stage::pre_update, sortedlist>;
    using updates      = _staged_list<stage::update, sortedlist>;
    using postupdates  = _staged_list<stage::post_update, sortedlist>;
    using renders      = _staged_list<stage::render, sortedlist>;
    using lasts        = _staged_list<stage::last, sortedlist>;
    using shutdowns    = _staged_list<stage::shutdown, sortedlist>;

    // Keep stage order consistent
    using all = type_list<
        prestartups, startups, poststartups, firsts, preupdates, updates,
        postupdates, renders, lasts, shutdowns>;
};

// same stage
template <typename Group>
struct _dispatch;
template <stage Stage, typename... SysInfo>
struct _dispatch<staged_type_list<Stage, SysInfo...>> {
    template <typename Curr, typename Remains>
    struct _patch;
    template <typename... Sys>
    struct _patch<type_list<Sys...>, type_list<>> {
        using type = type_list<Sys...>;
    };
    template <typename... SysLists, typename... Sys>
    struct _patch<type_list<SysLists...>, type_list<Sys...>> {
        template <typename Curr, typename List>
        struct _satisfy;
        template <typename... S>
        struct _satisfy<type_list<S...>, type_list<>> {
            using type = type_list<S...>;
        };
        template <typename... S, typename T, typename... Others>
        struct _satisfy<type_list<S...>, type_list<T, Others...>> {
            using current = std::conditional_t<
                _add_system::_is_before_v<T, type_list<Others...>>,
                type_list<S..., T>, type_list<S...>>;
            using type = typename _satisfy<current, type_list<Others...>>::type;
        };

        using filted_type = _satisfy<type_list<>, type_list<Sys...>>::type;
        using removed_type =
            type_list_erase_in_t<type_list<Sys...>, filted_type>;
        using type = typename _patch<
            type_list<SysLists..., filted_type>, removed_type>::type;
    };

    using tlist    = type_list<SysInfo...>;
    using run_list = typename _patch<type_list<>, tlist>::type;
    using type     = _to_staged_t<Stage, run_list>;
};

template <typename Group>
using _dispatch_t = typename _dispatch<Group>::type;

template <typename Grouped>
struct _handle_groups {
    using prestartups  = _dispatch_t<typename Grouped::prestartups>;
    using startups     = _dispatch_t<typename Grouped::startups>;
    using poststartups = _dispatch_t<typename Grouped::poststartups>;
    using firsts       = _dispatch_t<typename Grouped::firsts>;
    using preupdates   = _dispatch_t<typename Grouped::preupdates>;
    using updates      = _dispatch_t<typename Grouped::updates>;
    using postupdates  = _dispatch_t<typename Grouped::postupdates>;
    using renders      = _dispatch_t<typename Grouped::renders>;
    using lasts        = _dispatch_t<typename Grouped::lasts>;
    using shutdowns    = _dispatch_t<typename Grouped::shutdowns>;

    using all = type_list<
        prestartups, startups, poststartups, firsts, preupdates, updates,
        postupdates, renders, lasts, shutdowns>;
};

template <typename SysInfoList>
struct _fetch_resources;
template <typename... SysInfo>
struct _fetch_resources<type_list<SysInfo...>> {
    using type = unique_type_list_t<type_list_convert_t<
        std::remove_cvref,
        type_list_cat_t<typename SysInfo::fn_traits::resource...>>>;
};
template <typename SysInfoList>
struct _fetch_locals;
template <typename... SysInfo>
struct _fetch_locals<type_list<SysInfo...>> {
    template <typename Ty>
    using _predicate_type = std::negation<internal::_is_empty_sys_tuple<Ty>>;
    using type            = type_list_filt_t<
                   _predicate_type, type_list<typename SysInfo::fn_traits::local...>>;
};

template <world_descriptor Descriptor>
struct descriptor_traits {
    using sysinfo   = typename Descriptor::sysinfo;
    using grouped   = _group_by_stage<sysinfo>;
    using runlists  = _handle_groups<grouped>;
    using locals    = _fetch_locals<sysinfo>::type;
    using resources = _fetch_resources<sysinfo>::type;
};

} // namespace _descriptor_traits

using _descriptor_traits::descriptor_traits;

} // namespace neutron
