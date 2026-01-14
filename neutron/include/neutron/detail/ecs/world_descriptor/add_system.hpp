// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <neutron/detail/metafn/empty.hpp>
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/local.hpp"
#include "neutron/detail/ecs/res.hpp"
#include "neutron/detail/ecs/stage.hpp"
#include "neutron/detail/ecs/world_descriptor/fwd.hpp"

namespace neutron {

template <stage Stage, auto Fn, typename... Requires>
struct _add_system_t :
    public descriptor_adaptor_closure<_add_system_t<Stage, Fn, Requires...>> {

    template <world_descriptor Descriptor>
    consteval auto operator()(Descriptor descriptor) const noexcept {
        return typename Descriptor::template add_system_t<
            Stage, Fn, Requires...>{};
    }
};

template <auto Stage, auto Fn, typename... Requires>
constexpr _add_system_t<Stage, Fn, Requires...> add_system;

template <auto... Systems>
struct before {};

template <auto... Systems>
struct after {};

namespace _add_system {

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
            _systuple, type_list_first_t<type_list_filt_nempty_t<
                           internal::_is_local, arg_list, type_list<local<>>>>>;
};

template <stage Stage, auto Fn, typename... Requires>
struct sysinfo {
    static constexpr stage stage = Stage;
    static constexpr auto fn     = Fn;
    using fn_traits              = _fn_traits<Fn>;
    using requirements           = type_list<Requires...>;
};

template <typename, typename>
struct sysinfo_cmp_less;
template <
    stage LStage, auto LFn, typename... LRequires, stage RStage, auto RFn,
    typename... RRequires>
struct sysinfo_cmp_less<
    sysinfo<LStage, LFn, LRequires...>, sysinfo<RStage, RFn, RRequires...>> {
    static constexpr bool value = LStage < RStage;
    constexpr bool operator()() const noexcept { return LStage < RStage; }
};

template <typename SysInfo, typename InfoOrList>
struct _has_before;
template <
    stage Stage, auto LFn, typename... LRequires, auto RFn,
    typename... RRequires>
struct _has_before<
    sysinfo<Stage, LFn, LRequires...>, sysinfo<Stage, RFn, RRequires...>> {
    static_assert(LFn != RFn, "Cannot compare with same system");

    template <typename Require>
    using _predicate_type       = is_specific_value_list<before, Require>;
    static constexpr bool value = [] {
        using result =
            type_list_filt_t<_predicate_type, type_list<LRequires...>>;
        if constexpr (is_empty_template_v<result>) {
            return false;
        } else {
            using before_list = type_list_first_t<result>;
            return value_list_has_v<RFn, before_list>;
        }
    }();
};
template <
    stage Stage, auto Fn, typename... Requires,
    template <typename...> typename Template, typename... Info>
struct _has_before<sysinfo<Stage, Fn, Requires...>, Template<Info...>> {
    static constexpr bool value =
        (_has_before<sysinfo<Stage, Fn, Requires...>, Info>::value || ...);
};
template <typename SysInfo, typename InfoOrList>
constexpr bool _has_before_v = _has_before<SysInfo, InfoOrList>::value;

template <typename SysInfo, typename InfoOrList>
struct _has_after;
template <
    stage Stage, auto LFn, typename... LRequires, auto RFn,
    typename... RRequires>
struct _has_after<
    sysinfo<Stage, LFn, LRequires...>, sysinfo<Stage, RFn, RRequires...>> {
    static_assert(LFn != RFn, "Cannot compare with same system");

    template <typename Require>
    using _predicate_type       = is_specific_value_list<after, Require>;
    static constexpr bool value = [] {
        using result =
            type_list_filt_t<_predicate_type, type_list<LRequires...>>;
        if constexpr (is_empty_template_v<result>) {
            return false;
        } else {
            using after_list = type_list_first_t<result>;
            return value_list_has_v<RFn, after_list>;
        }
    }();
};
template <
    stage Stage, auto Fn, typename... Requires,
    template <typename...> typename Template, typename... Info>
struct _has_after<sysinfo<Stage, Fn, Requires...>, Template<Info...>> {
    static constexpr bool value =
        (_has_after<sysinfo<Stage, Fn, Requires...>, Info>::value || ...);
};
template <typename SysInfo, typename InfoOrList>
constexpr bool _has_after_v = _has_after<SysInfo, InfoOrList>::value;

template <typename SysInfo, typename InfoList>
struct _is_before;
template <
    typename SysInfo, template <typename...> typename Template,
    typename... Info>
struct _is_before<SysInfo, Template<Info...>> {
    using type_list             = Template<Info...>;
    static constexpr bool value = !_has_after_v<SysInfo, type_list> &&
                                  !(_has_before_v<Info, SysInfo> || ...);
};
template <typename SysInfo, typename InfoList>
constexpr auto _is_before_v = _is_before<SysInfo, InfoList>::value;

template <typename SysInfo, typename InfoList>
struct _is_after;
template <
    typename SysInfo, template <typename...> typename Template,
    typename... Info>
struct _is_after<SysInfo, Template<Info...>> {
    static constexpr bool value =
        ((_has_after_v<SysInfo, Info> || _has_before_v<Info, SysInfo>) && ...);
};
template <typename SysInfo, typename InfoList>
constexpr auto _is_after_v = _is_after<SysInfo, InfoList>::value;

} // namespace _add_system

} // namespace neutron
