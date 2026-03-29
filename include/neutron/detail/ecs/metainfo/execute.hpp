// IWYU pragma: private, include "neutron/detail/ecs/metainfo.hpp"
#pragma once
#include <cstddef>
#include <type_traits>
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/metainfo/common.hpp"
#include "neutron/detail/ecs/world_descriptor/execute.hpp"

namespace neutron {

namespace _metainfo {

template <typename>
struct _is_execute_group_policy : std::false_type {};

template <typename>
struct _is_execute_always_policy : std::false_type {};

template <>
struct _is_execute_always_policy<_always_t> : std::true_type {};

template <size_t Index>
struct _is_execute_group_policy<_group_t<Index>> : std::true_type {};

template <typename>
struct _is_execute_individual_policy : std::false_type {};

template <>
struct _is_execute_individual_policy<_individual_t> : std::true_type {};

template <typename>
struct _is_execute_frequency_policy : std::false_type {};

template <double Freq>
struct _is_execute_frequency_policy<_frequency_t<Freq>> : std::true_type {};

template <typename>
struct _is_execute_dynamic_frequency_policy : std::false_type {};

template <double DefaultFreq>
struct _is_execute_dynamic_frequency_policy<
    _execute::_dynamic_frequency_t<DefaultFreq>> : std::true_type {};

template <typename Policy>
constexpr bool _is_execute_policy_v =
    _is_execute_group_policy<Policy>::value ||
    _is_execute_always_policy<Policy>::value ||
    _is_execute_individual_policy<Policy>::value ||
    _is_execute_frequency_policy<Policy>::value ||
    _is_execute_dynamic_frequency_policy<Policy>::value;

template <auto Policy>
struct _is_execute_policy_value :
    std::bool_constant<
        _is_execute_policy_v<std::remove_cvref_t<decltype(Policy)>>> {};

template <auto Policy>
struct _is_execute_group_value :
    _is_execute_group_policy<std::remove_cvref_t<decltype(Policy)>> {};

template <auto Policy>
struct _is_execute_always_value :
    _is_execute_always_policy<std::remove_cvref_t<decltype(Policy)>> {};

template <auto Policy>
struct _is_execute_individual_value :
    _is_execute_individual_policy<std::remove_cvref_t<decltype(Policy)>> {};

template <auto Policy>
struct _is_execute_frequency_value :
    _is_execute_frequency_policy<std::remove_cvref_t<decltype(Policy)>> {};

template <auto Policy>
struct _is_execute_dynamic_frequency_value :
    _is_execute_dynamic_frequency_policy<
        std::remove_cvref_t<decltype(Policy)>> {};

template <typename>
struct _execute_group_index;

template <size_t Index>
struct _execute_group_index<_group_t<Index>> :
    std::integral_constant<size_t, Index> {};

template <typename>
struct _execute_frequency_interval;

template <double Freq>
struct _execute_frequency_interval<_frequency_t<Freq>> {
    static constexpr double value = Freq;
};

template <typename ValueList>
struct _execinfo_from_value_list;

template <template <auto...> typename Template, auto... Policies>
struct _execinfo_from_value_list<Template<Policies...>> {
    using type = tagged_value_list<_execute::exec_tag_t, Policies...>;
};

template <typename ValueList>
using _execinfo_from_value_list_t =
    typename _execinfo_from_value_list<ValueList>::type;

template <typename, bool DefaultGroup, bool DefaultAlways>
struct _normalize_execinfo;

template <bool DefaultGroup, bool DefaultAlways, auto... Policies>
struct _normalize_execinfo<
    tagged_value_list<_execute::exec_tag_t, Policies...>, DefaultGroup,
    DefaultAlways> {
    using raw_policy_list = value_list<Policies...>;

    using group_policies =
        value_list_filt_t<_is_execute_group_value, raw_policy_list>;
    using always_policies =
        value_list_filt_t<_is_execute_always_value, raw_policy_list>;
    using individual_policies =
        value_list_filt_t<_is_execute_individual_value, raw_policy_list>;
    using frequency_policies =
        value_list_filt_t<_is_execute_frequency_value, raw_policy_list>;
    using dynamic_frequency_policies =
        value_list_filt_t<_is_execute_dynamic_frequency_value, raw_policy_list>;

private:
    template <template <typename> typename Predicate>
    static consteval size_t _count() noexcept {
        return (
            size_t(Predicate<std::remove_cvref_t<decltype(Policies)>>::value) +
            ... + 0);
    }

    static consteval auto _group_policy() noexcept {
        if constexpr (is_empty_template_v<group_policies>) {
            return group<0>;
        } else {
            return value_list_element_v<0, group_policies>;
        }
    }

public:
    static constexpr bool has_only_known_policies =
        (_is_execute_policy_v<std::remove_cvref_t<decltype(Policies)>> && ...);

    static constexpr size_t group_count  = _count<_is_execute_group_policy>();
    static constexpr size_t always_count = _count<_is_execute_always_policy>();
    static constexpr size_t individual_count =
        _count<_is_execute_individual_policy>();
    static constexpr size_t frequency_count =
        _count<_is_execute_frequency_policy>();
    static constexpr size_t dynamic_frequency_count =
        _count<_is_execute_dynamic_frequency_policy>();

    static constexpr bool value =
        has_only_known_policies && group_count <= 1 && always_count <= 1 &&
        individual_count <= 1 && frequency_count <= 1 &&
        dynamic_frequency_count <= 1 &&
        (always_count + frequency_count + dynamic_frequency_count) <= 1 &&
        !(group_count != 0 && individual_count != 0);

private:
    using domain_policy_list = std::remove_pointer_t<decltype([] {
        if constexpr (!is_empty_template_v<individual_policies>) {
            return static_cast<
                value_list<value_list_element_v<0, individual_policies>>*>(
                nullptr);
        } else if constexpr (
            !is_empty_template_v<group_policies> || DefaultGroup) {
            return static_cast<value_list<_group_policy()>*>(nullptr);
        } else {
            return static_cast<value_list<>*>(nullptr);
        }
    }())>;

    using update_policy_list = std::remove_pointer_t<decltype([] {
        if constexpr (!is_empty_template_v<dynamic_frequency_policies>) {
            return static_cast<value_list<
                value_list_element_v<0, dynamic_frequency_policies>>*>(nullptr);
        } else if constexpr (!is_empty_template_v<frequency_policies>) {
            return static_cast<
                value_list<value_list_element_v<0, frequency_policies>>*>(
                nullptr);
        } else if constexpr (!is_empty_template_v<always_policies>) {
            return static_cast<
                value_list<value_list_element_v<0, always_policies>>*>(nullptr);
        } else if constexpr (DefaultAlways) {
            return static_cast<value_list<always>*>(nullptr);
        } else {
            return static_cast<value_list<>*>(nullptr);
        }
    }())>;

public:
    using type = _execinfo_from_value_list_t<
        value_list_cat_t<domain_policy_list, update_policy_list>>;
};

template <typename ExecInfo>
struct _validate_execinfo : _normalize_execinfo<ExecInfo, true, true> {};

template <typename ExecInfo>
struct _validate_local_execinfo : _normalize_execinfo<ExecInfo, false, true> {
    using base = _normalize_execinfo<ExecInfo, false, true>;

    static constexpr bool value =
        base::value && base::dynamic_frequency_count == 0;
    using type = typename base::type;
};

template <typename>
struct execinfo_traits;

template <auto... Policies>
struct execinfo_traits<tagged_value_list<_execute::exec_tag_t, Policies...>> {
    using tagged_type = tagged_value_list<_execute::exec_tag_t, Policies...>;
    using policy_list = value_list<Policies...>;
    using group_policies =
        value_list_filt_t<_is_execute_group_value, policy_list>;
    using always_policies =
        value_list_filt_t<_is_execute_always_value, policy_list>;
    using individual_policies =
        value_list_filt_t<_is_execute_individual_value, policy_list>;
    using frequency_policies =
        value_list_filt_t<_is_execute_frequency_value, policy_list>;
    using dynamic_frequency_policies =
        value_list_filt_t<_is_execute_dynamic_frequency_value, policy_list>;

    static constexpr bool is_individual =
        !is_empty_template_v<individual_policies>;
    static constexpr bool is_grouped = !is_empty_template_v<group_policies>;
    static constexpr bool is_always  = !is_empty_template_v<always_policies>;
    static constexpr bool has_execution_policy = is_individual || is_grouped;
    static constexpr bool has_frequency =
        !is_empty_template_v<frequency_policies>;
    static constexpr bool has_dynamic_frequency =
        !is_empty_template_v<dynamic_frequency_policies>;

    using group_policy = std::remove_pointer_t<decltype([] {
        if constexpr (is_grouped) {
            return static_cast<std::remove_cvref_t<
                decltype(value_list_element_v<0, group_policies>)>*>(nullptr);
        } else {
            return static_cast<void*>(nullptr);
        }
    }())>;

    using always_policy = std::remove_pointer_t<decltype([] {
        if constexpr (is_always) {
            return static_cast<std::remove_cvref_t<
                decltype(value_list_element_v<0, always_policies>)>*>(nullptr);
        } else {
            return static_cast<void*>(nullptr);
        }
    }())>;

    using frequency_policy = std::remove_pointer_t<decltype([] {
        if constexpr (has_frequency) {
            return static_cast<std::remove_cvref_t<
                decltype(value_list_element_v<0, frequency_policies>)>*>(
                nullptr);
        } else {
            return static_cast<void*>(nullptr);
        }
    }())>;

    using execution_policy = std::remove_pointer_t<decltype([] {
        if constexpr (is_individual) {
            return static_cast<std::remove_cvref_t<
                decltype(value_list_element_v<0, individual_policies>)>*>(
                nullptr);
        } else if constexpr (is_grouped) {
            return static_cast<group_policy*>(nullptr);
        } else {
            return static_cast<void*>(nullptr);
        }
    }())>;

    static constexpr size_t group_index = [] {
        if constexpr (is_grouped) {
            return _execute_group_index<group_policy>::value;
        } else {
            return size_t(0);
        }
    }();

    static constexpr double frequency_interval = [] {
        if constexpr (has_frequency) {
            return _execute_frequency_interval<frequency_policy>::value;
        } else {
            return 0.0;
        }
    }();
};

template <bool Valid, typename ParentType, typename ChildType>
struct _merge_execinfo_type;

template <typename ParentType, typename ChildType>
struct _merge_execinfo_type<false, ParentType, ChildType> {
    using type = tagged_value_list<_execute::exec_tag_t>;
};

template <auto... ParentPolicies, auto... ChildPolicies>
struct _merge_execinfo_type<
    true, tagged_value_list<_execute::exec_tag_t, ParentPolicies...>,
    tagged_value_list<_execute::exec_tag_t, ChildPolicies...>> {
    using parent_type =
        tagged_value_list<_execute::exec_tag_t, ParentPolicies...>;
    using child_type =
        tagged_value_list<_execute::exec_tag_t, ChildPolicies...>;
    using parent_traits = execinfo_traits<parent_type>;
    using child_traits  = execinfo_traits<child_type>;

private:
    static consteval auto _domain_policy() noexcept {
        if constexpr (child_traits::is_individual) {
            return value_list_element_v<
                0, typename child_traits::individual_policies>;
        } else if constexpr (child_traits::is_grouped) {
            return value_list_element_v<
                0, typename child_traits::group_policies>;
        } else if constexpr (parent_traits::is_individual) {
            return value_list_element_v<
                0, typename parent_traits::individual_policies>;
        } else {
            return value_list_element_v<
                0, typename parent_traits::group_policies>;
        }
    }

    using domain_policy_list = value_list<_domain_policy()>;

    using update_policy_list = std::remove_pointer_t<decltype([] {
        if constexpr (child_traits::has_dynamic_frequency) {
            return static_cast<value_list<value_list_element_v<
                0, typename child_traits::dynamic_frequency_policies>>*>(
                nullptr);
        } else if constexpr (child_traits::has_frequency) {
            return static_cast<value_list<value_list_element_v<
                0, typename child_traits::frequency_policies>>*>(nullptr);
        } else if constexpr (child_traits::is_always) {
            return static_cast<value_list<value_list_element_v<
                0, typename child_traits::always_policies>>*>(nullptr);
        } else if constexpr (parent_traits::has_dynamic_frequency) {
            return static_cast<value_list<value_list_element_v<
                0, typename parent_traits::dynamic_frequency_policies>>*>(
                nullptr);
        } else if constexpr (parent_traits::has_frequency) {
            return static_cast<value_list<value_list_element_v<
                0, typename parent_traits::frequency_policies>>*>(nullptr);
        } else if constexpr (parent_traits::is_always) {
            return static_cast<value_list<value_list_element_v<
                0, typename parent_traits::always_policies>>*>(nullptr);
        } else {
            return static_cast<value_list<>*>(nullptr);
        }
    }())>;

public:
    using type = _execinfo_from_value_list_t<
        value_list_cat_t<domain_policy_list, update_policy_list>>;
};

template <typename ParentExecInfo, typename ChildExecInfo>
struct merge_execinfo {
    using parent_validation = _validate_execinfo<ParentExecInfo>;
    using child_validation  = _validate_local_execinfo<ChildExecInfo>;

    using parent_type   = typename parent_validation::type;
    using child_type    = typename child_validation::type;
    using parent_traits = execinfo_traits<parent_type>;
    using child_traits  = execinfo_traits<child_type>;

    static constexpr bool domain_compatible =
        !(parent_traits::is_individual && child_traits::is_grouped);
    static constexpr bool value = parent_validation::value &&
                                  child_validation::value && domain_compatible;

    using type =
        typename _merge_execinfo_type<value, parent_type, child_type>::type;
};

template <typename ParentExecInfo, typename ChildExecInfo>
struct _checked_merge_execinfo {
    using merge = merge_execinfo<ParentExecInfo, ChildExecInfo>;

    static_assert(
        merge::value,
        "invalid execute policies: group conflicts with individual, world "
        "individual cannot be overridden by a system group, systems default "
        "to always, and local execute policies cannot use dynamic_frequency");

    using type = typename merge::type;
};

template <typename ParentExecInfo, typename ChildExecInfo>
using merge_execinfo_t =
    typename _checked_merge_execinfo<ParentExecInfo, ChildExecInfo>::type;

template <typename Descriptor>
struct fetch_execinfo {
    using raw_type   = fetch_vinfo_t<_execute::exec_tag_t, Descriptor>;
    using validation = _validate_execinfo<raw_type>;

    static_assert(
        validation::value,
        "invalid execute policies: group conflicts with individual, only one "
        "update mode may be specified, and frequency conflicts with "
        "dynamic_frequency");

    using type = typename validation::type;
};

template <typename Descriptor>
using fetch_execinfo_t = typename fetch_execinfo<Descriptor>::type;

template <typename Descriptor>
struct execute_info : execinfo_traits<fetch_execinfo_t<Descriptor>> {};

} // namespace _metainfo

using _metainfo::execute_info;
using _metainfo::execinfo_traits;
using _metainfo::fetch_execinfo;
using _metainfo::fetch_execinfo_t;
using _metainfo::merge_execinfo;
using _metainfo::merge_execinfo_t;

} // namespace neutron
