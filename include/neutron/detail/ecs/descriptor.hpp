#pragma once
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include "neutron/detail/metafn/definition.hpp"
#include "neutron/detail/metafn/element.hpp"
#include "neutron/inplace_vector.hpp"

namespace neutron {

struct description_tag {};

template <typename... Args>
struct world_descriptor_t;

namespace internal {

template <typename T>
constexpr bool _is_world_descriptor = false;
template <typename... Args>
constexpr bool _is_world_descriptor<world_descriptor_t<Args...>> = true;

} // namespace internal

template <typename T>
concept description =
    std::derived_from<T, description_tag> && !internal::_is_world_descriptor<T>;

template <description... Args>
struct world_descriptor_t<Args...> : Args... {};

template <typename T>
concept descriptor = internal::_is_world_descriptor<std::remove_cvref_t<T>>;

inline constexpr world_descriptor_t<> world_desc;

template <descriptor Descriptor, description Description>
consteval auto operator|(Descriptor desc, Description description) noexcept {
    return description(desc);
}

template <typename Desc1, typename Desc2>
struct _description_compose : description_tag {
    template <descriptor Desc>
    consteval auto operator()(Desc desc) const noexcept {
        return Desc2{}(Desc1{}(desc));
    }
};

template <description Desc1, description Desc2>
consteval auto operator|(Desc1, Desc2) noexcept {
    return _description_compose<Desc1, Desc2>{};
}

enum class stage : std::uint8_t {
    prestartup,
    startup,
    poststartup,
    first,
    events,
    preupdate,
    update,
    postupdate,
    render,
    last,
    shutdown
};

inline namespace _metafn {

template <typename, typename>
constexpr bool _has_same_template = false;
template <template <auto...> typename Tmp, auto... Tys1, auto... Tys2>
constexpr bool _has_same_template<Tmp<Tys1...>, Tmp<Tys2...>> = true;
template <
    template <typename...> typename Tmp, typename... Tys1, typename... Tys2>
constexpr bool _has_same_template<Tmp<Tys1...>, Tmp<Tys2...>> = true;

template <typename List, typename T>
struct _cat_if_same_template {
    using type = List;
};
template <
    template <typename...> typename Tmp, typename... Tys, typename... Args>
struct _cat_if_same_template<Tmp<Tys...>, Tmp<Args...>> {
    using type = Tmp<Tys..., Args...>;
};
template <
    template <stage, typename...> typename Tmp, stage Stage, typename... Tys,
    typename... Args>
struct _cat_if_same_template<Tmp<Stage, Tys...>, Tmp<Stage, Args...>> {
    using type = Tmp<Stage, Tys..., Args...>;
};
template <
    template <stage, auto...> typename Tmp, stage Stage, auto... Vals,
    auto... Args>
struct _cat_if_same_template<Tmp<Stage, Vals...>, Tmp<Stage, Args...>> {
    using type = Tmp<Stage, Vals..., Args...>;
};

template <typename TypeList, typename T>
struct cat_or_insert;
template <template <typename...> typename Tmp, typename... Tys, typename T>
requires(false || ... || _has_same_template<Tys, T>)
struct cat_or_insert<Tmp<Tys...>, T> {
    using type = Tmp<typename _cat_if_same_template<Tys, T>::type...>;
};
template <template <typename...> typename Tmp, typename... Tys, typename T>
requires(!(false || ... || _has_same_template<Tys, T>))
struct cat_or_insert<Tmp<Tys...>, T> {
    using type = Tmp<Tys..., T>;
};
template <typename TypeList, typename T>
using cat_or_insert_t = typename cat_or_insert<TypeList, T>::type;

template <typename TypeList, typename T>
struct get_if_exist;
template <template <typename...> typename Tmp, typename... Tys, typename T>
struct get_if_exist<Tmp<Tys...>, T> {
    using type = Tmp<>;
};

} // namespace _metafn

template <typename... Features>
struct _enabled_features_t : description_tag, Features... {
    template <descriptor Desc>
    consteval auto operator()(Desc) const noexcept {
        return cat_or_insert_t<Desc, _enabled_features_t<Features...>>();
    }
};

struct _enable_events_t {
    static constexpr bool enabled_events = true;
};
inline constexpr _enabled_features_t<_enable_events_t> enable_events;

struct _enable_render_t {
    static constexpr bool enabled_render = true;
};
inline constexpr _enabled_features_t<_enable_render_t> enable_render;

template <auto... Fn>
struct _before_t {};
template <auto... Fn>
inline constexpr _before_t<Fn...> before;
template <auto... Fn>
struct _after_t {};
template <auto... Fn>
inline constexpr _after_t<Fn...> after;

template <typename Fn, auto... Requires>
struct system_spec;
template <typename Ret, typename... Args, auto... Requires>
struct system_spec<Ret (*)(Args...), Requires...> {
    using fn_t = Ret (*)(Args...);
    fn_t fn;
    consteval system_spec(fn_t fn, decltype(Requires)...) noexcept : fn(fn) {}
};
template <typename Ret, typename... Args, auto... Requires>
struct system_spec<Ret (*)(Args...) noexcept, Requires...> {
    using fn_t = Ret (*)(Args...) noexcept;
    fn_t fn;
    consteval system_spec(fn_t fn, decltype(Requires)...) noexcept : fn(fn) {}
};

template <typename Fn, typename... Requires>
system_spec(Fn, Requires...) -> system_spec<Fn, Requires{}...>;

template <typename Fn>
system_spec(Fn) -> system_spec<Fn>;

struct _individual_t {};
inline constexpr _individual_t individual;

struct interval {
    double val = 0.0;
};

template <typename Tasks>
struct graph_builder;

template <stage Stage, system_spec... Specs>
struct _add_systems_t;

template <system_spec Spec, auto Fn>
consteval bool _task_spec_matches() {
    if constexpr (requires { Spec.fn == Fn; }) {
        return Spec.fn == Fn;
    } else {
        return false;
    }
}

namespace internal {

template <std::size_t, typename T>
constexpr void _add_requirement(auto&&, T) noexcept {}

} // namespace internal

template <stage Stage, system_spec... Spec>
class graph_builder<_add_systems_t<Stage, Spec...>> {
    static constexpr std::size_t count = sizeof...(Spec);
    using graph_t = std::array<inplace_vector<std::size_t, count>, count>;

    template <auto Fn>
    static consteval std::size_t _task_index() noexcept {
        std::size_t result  = count;
        std::size_t current = 0;
        ((result == count && _task_spec_matches<Spec.fn, Fn>()
              ? result = current
              : result,
          ++current),
         ...);
        return result;
    }

    template <std::size_t Task, auto Fn>
    static constexpr void _add_after_edge(graph_t& graph) noexcept {
        constexpr auto dependency = _task_index<Fn>();
        static_assert(
            dependency < count, "after() references a task that was not added");
        graph[Task].emplace_back(dependency);
    }

    template <std::size_t Task, auto Fn>
    static constexpr void _add_before_edge(graph_t& graph) noexcept {
        constexpr auto dependent = _task_index<Fn>();
        static_assert(
            dependent < count, "before() references a task that was not added");
        graph[dependent].emplace_back(Task);
    }

    template <std::size_t Task, auto... Fn>
    static constexpr void
        _add_requirement(graph_t& graph, _after_t<Fn...>) noexcept {
        (_add_after_edge<Task, Fn>(graph), ...);
    }

    template <std::size_t Task, auto... Fn>
    static constexpr void
        _add_requirement(graph_t& graph, _before_t<Fn...>) noexcept {
        (_add_before_edge<Task, Fn>(graph), ...);
    }

    template <std::size_t Task, typename Fn, auto... Req>
    static constexpr void
        _add_task_edges(graph_t& graph, system_spec<Fn, Req...>) noexcept {
        using ::neutron::internal::_add_requirement;
        (_add_requirement<Task>(graph, Req), ...);
    }

public:
    static consteval auto make() noexcept {
        return []<std::size_t... Is>(std::index_sequence<Is...>) {
            graph_t graph{};
            (_add_task_edges<Is>(graph, Spec), ...);
            return graph;
        }(std::make_index_sequence<count>());
    }
};

enum class accessibility : std::uint8_t {
    read,
    write
};
template <typename T>
struct request_accessibility;
template <typename T>
requires std::same_as<std::remove_cvref_t<T>, T>
struct request_accessibility<T> {
    using type                             = T;
    static constexpr accessibility ability = accessibility::read;
};
template <typename T>
struct request_accessibility<const T> : request_accessibility<T> {};
template <typename T>
struct request_accessibility<const T&> : request_accessibility<T> {};
template <typename T>
struct request_accessibility<T&> {
    using type                             = T;
    static constexpr accessibility ability = accessibility::write;
};
template <typename T>
struct param_spec {
    using accessibilities = type_list<>;
};

template <stage Stage, system_spec... Specs>
class _add_systems_t : public description_tag {
    using list_t = value_list<Specs...>;
    using iseq_t = std::make_index_sequence<sizeof...(Specs)>;

    template <std::size_t I, system_spec Spec>
    static consteval bool _has_match() noexcept {
        return []<std::size_t... J>(std::index_sequence<J...>) {
            return (
                _task_spec_matches<
                    Spec, value_list_element_v<I + J, list_t>.fn>() ||
                ...);
        }(std::make_index_sequence<sizeof...(Specs) - I>());
    }

    static consteval bool _is_unique() noexcept {
        return ![]<std::size_t... I>(std::index_sequence<I...>) {
            return (
                _has_match<I + 1, value_list_element_v<I, list_t>>() || ...);
        }(iseq_t());
    }

    static_assert(_is_unique());

    static consteval auto _cal_closure() {
        constexpr auto count = sizeof...(Specs);
        auto graph           = graph_builder<_add_systems_t>::make();
        std::array<std::array<bool, count>, count> closure{};
        for (std::size_t i = 0; i < count; ++i) {
            for (auto task : graph[i]) {
                closure[i][task] = true;
            }
        }

        for (std::size_t i = 0; i < count; ++i) {
            for (std::size_t j = 0; j < count; ++j) {
                if (closure[i][j]) {
                    for (std::size_t k = 0; k < count; ++k) {
                        closure[i][k] |= closure[j][k];
                    }
                }
            }
        }

        return closure;
    }

    static consteval bool _has_loop() noexcept {
        auto closure = _cal_closure();
        for (std::size_t i = 0; i < closure.size(); ++i) {
            if (closure[i][i]) {
                return true;
            }
        }
        return false;
    }

    static_assert(
        !_has_loop(), "there is a dependency loop. please check the "
                      "use of 'before' and 'after'");

public:
    template <typename Desc>
    consteval auto operator()(Desc) const noexcept {
        static_assert(
            Stage != stage::events || requires { Desc::enabled_events; },
            "events should be enabled before adding events systems");

        static_assert(
            Stage != stage::render || requires { Desc::enabled_render; },
            "render should be enabled before adding render systems");

        return cat_or_insert_t<Desc, _add_systems_t>();
    }
};

template <stage Stage, system_spec... Specs>
inline constexpr _add_systems_t<Stage, Specs...> add_systems;

template <typename TypeList, typename T>
struct _count_if_is_type;
template <template <typename...> typename Tmp, typename... Args, typename T>
struct _count_if_is_type<Tmp<Args...>, T> {
    static constexpr std::size_t value = (std::same_as<Args, T> + ...);
};
template <template <auto...> typename Tmp, auto... Args, typename T>
struct _count_if_is_type<Tmp<Args...>, T> {
    static constexpr std::size_t value =
        (std::same_as<std::remove_cvref_t<decltype(Args)>, T> + ...);
};

inline constexpr auto dynamic_interval = interval{ -1.0 };

template <std::size_t I>
struct _group_t {};
template <std::size_t I>
inline constexpr _group_t<I> group;

template <auto... Args>
struct _execute_t : description_tag {

    /* no or only one interval spec
     * execute individual or in group, defaultly in group<0>
     */

    using list_t = value_list<Args...>;

    // 0 or 1 if well-formed
    static_assert(
        _count_if_is_type<list_t, interval>::value <= 1,
        "has already existed an interval spec");

    // 0 or 1 if well-formed
    static constexpr std::size_t _individual_count =
        _count_if_is_type<list_t, _individual_t>::value;

    static_assert(
        _individual_count <= 1, "'individual' could only be apply once");

    // 0 or 1 if well-formed
    static constexpr std::size_t _group_count =
        (_has_same_template<std::remove_cvref_t<decltype(Args)>, _group_t<0>> +
         ...);

    static_assert(_group_count <= 1, "a world could only belongs to one group");

    static_assert(
        (_individual_count + _group_count) <= 1,
        "'individual' and 'group' could not appear at same time");

    static constexpr double _interval = []() -> double {
        if (_individual_count == 0) {
            return 0.0;
        }
        //
    }();

    template <descriptor Desc>
    consteval auto operator()(Desc) const noexcept {
        return cat_or_insert_t<Desc, _execute_t>();
    }
};

template <auto... Args>
inline constexpr _execute_t<Args...> execute;

} // namespace neutron
