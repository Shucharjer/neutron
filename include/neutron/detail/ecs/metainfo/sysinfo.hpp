// IWYU pragma: private, include "neutron/detail/ecs/metainfo.hpp"
#pragma once
#include "neutron/detail/ecs/global.hpp"
#include "neutron/detail/ecs/insertion.hpp"
#include "neutron/detail/ecs/local.hpp"
#include "neutron/detail/ecs/metainfo/common.hpp"
#include "neutron/detail/ecs/metainfo/execute.hpp"
#include "neutron/detail/ecs/query.hpp"
#include "neutron/detail/ecs/res.hpp"
#include "neutron/detail/ecs/systuple.hpp"
#include "neutron/detail/ecs/world_descriptor/add_systems.hpp"
#include "neutron/detail/type_traits/same_cvref.hpp"

namespace neutron {

namespace _metainfo {

using enum stage;

template <stage Stage, typename TypeList>
struct fetch_sysinfo {
    using type = fetch_vinfo_t<sys_tag_t<Stage>, TypeList>;
};

template <stage Stage, typename TypeList>
using fetch_sysinfo_t = typename fetch_sysinfo<Stage, TypeList>::type;

template <typename Descriptor>
struct sysinfo_holder {
    // clang-format off
    using prestartup_info  = fetch_sysinfo_t<pre_startup, Descriptor>;
    using startup_info     = fetch_sysinfo_t<startup, Descriptor>;
    using poststartup_info = fetch_sysinfo_t<post_startup, Descriptor>;

    using first_info       = fetch_sysinfo_t<first, Descriptor>;
    using events_info      = fetch_sysinfo_t<events, Descriptor>;

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

template <typename>
struct _validate_sysinfo;

template <>
struct _validate_sysinfo<type_list<>> {
    using arg_lists       = type_list<>;
    using query_list      = type_list<>;
    using querior_list    = type_list<>;
    using component_lists = type_list<>;
    using resource_lists  = type_list<>;
    using global_lists    = type_list<>;
    using insertion_lists = type_list<>;
    using access_keys     = type_list<>;
    using write_keys      = type_list<>;

    static constexpr bool value = true;
};

template <typename T>
struct to_component_list {
    using type = typename T::component_list;
};

template <typename T>
struct to_cached_querior {
    using type = typename T::cached_querior_type;
};

template <typename T>
struct to_resource_list;

template <resource_like... Resources>
struct to_resource_list<res<Resources...>> {
    using type =
        type_list_recurse_expose_t<bundle, type_list<Resources...>, same_cvref>;
};

template <typename T>
struct to_global_list;

template <typename... Globals>
struct to_global_list<global<Globals...>> {
    using type =
        type_list_recurse_expose_t<bundle, type_list<Globals...>, same_cvref>;
};

template <typename T>
struct to_insertion_list;

template <typename... Insertions>
struct to_insertion_list<insertion<Insertions...>> {
    using type = type_list_recurse_expose_t<
        bundle, type_list<Insertions...>, same_cvref>;
};

template <typename T>
struct _component_access_key;

template <typename T>
struct _resource_access_key;

template <typename T>
struct _global_access_key;

template <typename T>
struct _insertion_access_key;

template <typename T>
struct to_component_access_key {
    using type = _component_access_key<std::remove_cvref_t<T>>;
};

template <typename T>
struct to_resource_access_key {
    using type = _resource_access_key<std::remove_cvref_t<T>>;
};

template <typename T>
struct to_global_access_key {
    using type = _global_access_key<std::remove_cvref_t<T>>;
};

template <typename T>
struct to_insertion_access_key {
    using type = _insertion_access_key<std::remove_cvref_t<T>>;
};

template <
    template <typename> typename Predicate,
    template <typename> typename Converter, typename ArgList>
struct _extract_access_list {
    using matched = type_list_filt_t<Predicate, ArgList>;
    using type    = type_list_first_t<type_list_filt_nempty_t<
           always_true,
           type_list_list_cat_t<type_list_convert_t<Converter, matched>>,
           type_list<type_list<>>>>;
};

template <
    template <typename> typename Predicate,
    template <typename> typename Converter, typename ArgList>
using _extract_access_list_t =
    typename _extract_access_list<Predicate, Converter, ArgList>::type;

template <template <typename> typename Converter, typename AccessList>
using _access_key_list_t =
    unique_type_list_t<type_list_convert_t<Converter, AccessList>>;

template <template <typename> typename Converter, typename AccessList>
using _write_key_list_t = unique_type_list_t<
    type_list_convert_t<Converter, type_list_filt_t<writable, AccessList>>>;

template <typename>
struct _is_commands_arg : std::false_type {};

template <std_simple_allocator Alloc, bool Direct>
struct _is_commands_arg<basic_commands<Alloc, Direct>> : std::true_type {};

template <typename Ty>
constexpr bool _is_commands_arg_v =
    _is_commands_arg<std::remove_cvref_t<Ty>>::value;

template <typename WorldExecInfo, auto Desc, typename = decltype(Desc)>
struct _sys_traits;

template <typename WorldExecInfo, auto Desc, typename Sys, auto... Requires>
struct _sys_traits<WorldExecInfo, Desc, sysdesc<Sys, Requires...>> {
    using fn_traits = _fn_traits<Desc.fn>;

    static constexpr auto fn = Desc.fn;

    using arg_list      = typename fn_traits::arg_list;
    using resource      = typename fn_traits::resource;
    using local         = typename fn_traits::local;
    using requires_list = type_list<decltype(Requires)...>;
    using raw_requires  = value_list<Requires...>;

    using raw_execute_policies =
        value_list_filt_t<_is_execute_policy_value, raw_requires>;
    using raw_execute_info = _execinfo_from_value_list_t<raw_execute_policies>;
    using execute          = merge_execinfo_t<WorldExecInfo, raw_execute_info>;
    using execute_traits   = execinfo_traits<execute>;
    using execution_policy = typename execute_traits::execution_policy;

    static constexpr bool has_commands =
        !is_empty_template_v<type_list_filt_t<_is_commands_arg, arg_list>>;

    using query_list   = type_list_filt_t<internal::is_query, arg_list>;
    using querior_list = query_list;
    template <typename... Tys>
    using _query_tuple = _query::query_tuple<fn, Tys...>;
    using query_cache  = type_list_rebind_t<
         _query_tuple, type_list_convert_t<to_cached_querior, query_list>>;

    using component_list =
        _extract_access_list_t<internal::is_query, to_component_list, arg_list>;
    using resource_list =
        _extract_access_list_t<internal::_is_res, to_resource_list, arg_list>;
    using global_list =
        _extract_access_list_t<internal::_is_global, to_global_list, arg_list>;
    using insertion_list = _extract_access_list_t<
        internal::_is_insertion, to_insertion_list, arg_list>;

    using component_access_keys =
        _access_key_list_t<to_component_access_key, component_list>;
    using resource_access_keys =
        _access_key_list_t<to_resource_access_key, resource_list>;
    using global_access_keys =
        _access_key_list_t<to_global_access_key, global_list>;
    using insertion_access_keys =
        _access_key_list_t<to_insertion_access_key, insertion_list>;

    using component_write_keys =
        _write_key_list_t<to_component_access_key, component_list>;
    using resource_write_keys =
        _write_key_list_t<to_resource_access_key, resource_list>;
    using global_write_keys =
        _write_key_list_t<to_global_access_key, global_list>;
    using insertion_write_keys =
        _write_key_list_t<to_insertion_access_key, insertion_list>;

    using access_keys = unique_type_list_t<type_list_cat_t<
        component_access_keys, resource_access_keys, global_access_keys,
        insertion_access_keys>>;
    using write_keys  = unique_type_list_t<type_list_cat_t<
         component_write_keys, resource_write_keys, global_write_keys,
         insertion_write_keys>>;
};

template <typename Lhs, typename Rhs>
struct _access_list_intersects;

template <
    template <typename...> typename Template, typename... Lhs, typename Rhs>
struct _access_list_intersects<Template<Lhs...>, Rhs> :
    std::bool_constant<(false || ... || type_list_has_v<Lhs, Rhs>)> {};

template <typename Info, typename Other>
constexpr bool _has_before_v = type_list_has_v<
    _add_system::_before_t<Other::fn>, typename Info::requires_list>;

template <typename Info, typename Other>
constexpr bool _has_after_v = type_list_has_v<
    _add_system::_after_t<Other::fn>, typename Info::requires_list>;

template <typename Info, typename Other>
constexpr bool _direct_before_v =
    _has_before_v<Info, Other> || _has_after_v<Other, Info>;

template <typename Lhs, typename Rhs>
constexpr bool _different_group_v =
    Lhs::execute_traits::is_grouped && Rhs::execute_traits::is_grouped &&
    !std::same_as<
        typename Lhs::execution_policy, typename Rhs::execution_policy>;

template <typename Lhs, typename Rhs>
constexpr bool _cross_group_dependency_v =
    (_direct_before_v<Lhs, Rhs> || _direct_before_v<Rhs, Lhs>) &&
    _different_group_v<Lhs, Rhs>;

template <
    typename From, typename To, typename InfoList,
    typename Visited = type_list<From>>
struct _reachable;

template <
    typename From, typename To, template <typename...> typename Template,
    typename... Infos, typename Visited>
struct _reachable<From, To, Template<Infos...>, Visited> {
private:
    template <typename Next>
    static consteval bool _step() {
        if constexpr (
            type_list_has_v<Next, Visited> || !_direct_before_v<From, Next>) {
            return false;
        } else if constexpr (std::same_as<Next, To>) {
            return true;
        } else {
            return _reachable<
                Next, To, Template<Infos...>,
                type_list_cat_t<Visited, type_list<Next>>>::value;
        }
    }

public:
    static constexpr bool value = (_step<Infos>() || ...);
};

template <typename Lhs, typename Rhs, typename InfoList>
constexpr bool _ordered_v = _reachable<Lhs, Rhs, InfoList>::value ||
                            _reachable<Rhs, Lhs, InfoList>::value;

template <typename Info, typename InfoList>
struct _cyclic_dependency;

template <
    typename Info, template <typename...> typename Template, typename... Infos>
struct _cyclic_dependency<Info, Template<Infos...>> :
    std::bool_constant<
        _direct_before_v<Info, Info> ||
        (false || ... ||
         (!std::same_as<Info, Infos> && _direct_before_v<Info, Infos> &&
          _reachable<Infos, Info, Template<Infos...>>::value))> {};

template <typename Info, typename InfoList>
constexpr bool _cyclic_dependency_v = _cyclic_dependency<Info, InfoList>::value;

template <typename Lhs, typename Rhs>
constexpr bool _generic_access_conflict_v =
    _access_list_intersects<
        typename Lhs::write_keys, typename Rhs::access_keys>::value ||
    _access_list_intersects<
        typename Rhs::write_keys, typename Lhs::access_keys>::value;

template <typename Lhs, typename Rhs, typename InfoList>
constexpr bool _sysinfo_conflict_v =
    _generic_access_conflict_v<Lhs, Rhs> && !_ordered_v<Lhs, Rhs, InfoList>;

template <typename RemainingList, typename InfoList>
struct _validate_sysinfo_impl;

template <template <typename...> typename Template, typename InfoList>
struct _validate_sysinfo_impl<Template<>, InfoList> {
    static constexpr bool value = true;
};

template <
    template <typename...> typename Template, typename Head, typename... Tail,
    typename InfoList>
struct _validate_sysinfo_impl<Template<Head, Tail...>, InfoList> {
private:
    template <typename Other>
    static consteval bool _head_invalid() {
        return _sysinfo_conflict_v<Head, Other, InfoList> ||
               _cross_group_dependency_v<Head, Other> ||
               _cyclic_dependency_v<Head, InfoList>;
    }

public:
    static constexpr bool value =
        !(false || ... || _head_invalid<Tail>()) &&
        _validate_sysinfo_impl<Template<Tail...>, InfoList>::value;
};

template <typename TaggedSysInfo>
struct _sysinfo_traits_list;

template <stage Stage, auto... Desc>
struct _sysinfo_traits_list<tagged_value_list<sys_tag_t<Stage>, Desc...>> {
    using world_execute = fetch_execinfo_t<world_descriptor_t<>>;
    using type          = type_list<_sys_traits<world_execute, Desc>...>;
};

template <typename TaggedSysInfo>
using _sysinfo_traits_list_t =
    typename _sysinfo_traits_list<TaggedSysInfo>::type;

template <typename Descriptor, typename TaggedSysInfo>
struct _sysinfo_traits_list_for;

template <typename Descriptor, stage Stage, auto... Desc>
struct _sysinfo_traits_list_for<
    Descriptor, tagged_value_list<sys_tag_t<Stage>, Desc...>> {
    using world_execute = fetch_execinfo_t<Descriptor>;
    using type          = type_list<_sys_traits<world_execute, Desc>...>;
};

template <stage Stage, typename Descriptor>
using _sysinfo_traits_list_for_t = typename _sysinfo_traits_list_for<
    Descriptor, fetch_sysinfo_t<Stage, Descriptor>>::type;

template <auto... Fn>
struct _validate_sysinfo<type_list<_fn_traits<Fn>...>> {
    using arg_lists    = type_list_cat_t<typename _fn_traits<Fn>::arg_list...>;
    using query_list   = type_list_filt_t<internal::is_query, arg_lists>;
    using component_lists = type_list_first_t<type_list_filt_nempty_t<
        always_true,
        type_list_list_cat_t<
            type_list_convert_t<to_component_list, query_list>>,
        type_list<type_list<>>>>;
    using resource_lists  = type_list<>;
    using global_lists    = type_list<>;
    using insertion_lists = type_list<>;
    using access_keys     = type_list<>;
    using write_keys      = type_list<>;

    static constexpr bool value = true;
};

template <typename WorldExecInfo, auto... Desc>
struct _validate_sysinfo<type_list<_sys_traits<WorldExecInfo, Desc>...>> {
    using info_list = type_list<_sys_traits<WorldExecInfo, Desc>...>;

    using arg_lists =
        type_list_cat_t<typename _sys_traits<WorldExecInfo, Desc>::arg_list...>;
    using query_list      = type_list_filt_t<internal::is_query, arg_lists>;
    using querior_list    = query_list;
    using component_lists = type_list_first_t<type_list_filt_nempty_t<
        always_true,
        type_list_list_cat_t<
            type_list_convert_t<to_component_list, query_list>>,
        type_list<type_list<>>>>;
    using resource_lists  = type_list_cat_t<
         typename _sys_traits<WorldExecInfo, Desc>::resource_list...>;
    using global_lists = type_list_cat_t<
        typename _sys_traits<WorldExecInfo, Desc>::global_list...>;
    using insertion_lists = type_list_cat_t<
        typename _sys_traits<WorldExecInfo, Desc>::insertion_list...>;

    using access_keys = unique_type_list_t<type_list_cat_t<
        typename _sys_traits<WorldExecInfo, Desc>::access_keys...>>;
    using write_keys  = unique_type_list_t<type_list_cat_t<
         typename _sys_traits<WorldExecInfo, Desc>::write_keys...>>;

    static constexpr bool value =
        _validate_sysinfo_impl<info_list, info_list>::value;
};

template <stage Stage, auto... Desc>
struct _validate_sysinfo<tagged_value_list<sys_tag_t<Stage>, Desc...>> {
    using _traits_list =
        _sysinfo_traits_list_t<tagged_value_list<sys_tag_t<Stage>, Desc...>>;
    static constexpr bool value = _validate_sysinfo<_traits_list>::value;
};

template <stage Stage, typename Descriptor>
struct stage_sysinfo {
    using tagged_type   = fetch_sysinfo_t<Stage, Descriptor>;
    using world_execute = fetch_execinfo_t<Descriptor>;
    using traits_list   = _sysinfo_traits_list_for_t<Stage, Descriptor>;
    using validator     = _validate_sysinfo<traits_list>;

    using arg_lists       = typename validator::arg_lists;
    using query_list      = typename validator::query_list;
    using querior_list    = typename validator::querior_list;
    using component_lists = typename validator::component_lists;
    using resource_lists  = typename validator::resource_lists;
    using global_lists    = typename validator::global_lists;
    using insertion_lists = typename validator::insertion_lists;
    using access_keys     = typename validator::access_keys;
    using write_keys      = typename validator::write_keys;

    static constexpr bool value = validator::value;
};

} // namespace _metainfo

using _metainfo::stage_sysinfo;
using _metainfo::sysinfo_holder;

} // namespace neutron
