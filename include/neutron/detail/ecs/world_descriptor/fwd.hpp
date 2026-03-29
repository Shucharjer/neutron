// IWYU pragma: private, include "neutron/detail/ecs/world_descriptor.hpp"
#pragma once
#include <cstddef>
#include <type_traits>
#include "neutron/detail/pipeline.hpp"

namespace neutron {

template <typename... Descriptions>
struct world_descriptor_t {};

constexpr inline world_descriptor_t<> world_desc;

template <typename>
constexpr bool _is_world_descriptor = false;
template <typename... Content>
constexpr bool _is_world_descriptor<world_descriptor_t<Content...>> = true;

template <typename Descriptor>
concept world_descriptor =
    _is_world_descriptor<std::remove_cvref_t<Descriptor>>;

template <typename Derived>
struct descriptor_adaptor_closure : neutron::adaptor_closure<Derived> {};

template <typename Closure>
concept descriptor_closure =
    neutron::_adaptor_closure<Closure, descriptor_adaptor_closure>;

template <typename First, typename Second>
struct descriptor_closure_compose
    : descriptor_adaptor_closure<descriptor_closure_compose<First, Second>> {
    using self_type = descriptor_closure_compose;

    template <typename Self, typename Arg>
    using _invoke_result_t = std::invoke_result_t<
        same_cvref_t<Second, Self>,
        std::invoke_result_t<same_cvref_t<First, Self>, Arg>>;

    template <typename C1, typename C2>
    constexpr descriptor_closure_compose(C1&& closure1, C2&& closure2)
        : pair_(
              std::forward<C1>(closure1),
              std::forward<C2>(closure2)) {}

    template <typename Arg>
    requires std::is_invocable_v<same_cvref_t<First, const self_type&>, Arg> &&
             std::is_invocable_v<
                 same_cvref_t<Second, const self_type&>,
                 std::invoke_result_t<
                     same_cvref_t<First, const self_type&>, Arg>>
    consteval auto operator()(Arg&& arg) const&
        noexcept(noexcept((pair_.second())(pair_.first()(std::forward<Arg>(arg)))))
            -> _invoke_result_t<const self_type&, Arg> {
        return (pair_.second())(pair_.first()(std::forward<Arg>(arg)));
    }

    template <typename Arg>
    requires std::is_invocable_v<same_cvref_t<First, self_type&>, Arg> &&
             std::is_invocable_v<
                 same_cvref_t<Second, self_type&>,
                 std::invoke_result_t<same_cvref_t<First, self_type&>, Arg>>
    consteval auto operator()(Arg&& arg) &
        noexcept(noexcept((pair_.second())(pair_.first()(std::forward<Arg>(arg)))))
            -> _invoke_result_t<self_type&, Arg> {
        return (pair_.second())(pair_.first()(std::forward<Arg>(arg)));
    }

    template <typename Arg>
    requires std::is_invocable_v<same_cvref_t<First, self_type&&>, Arg> &&
             std::is_invocable_v<
                 same_cvref_t<Second, self_type&&>,
                 std::invoke_result_t<same_cvref_t<First, self_type&&>, Arg>>
    consteval auto operator()(Arg&& arg) &&
        noexcept(noexcept(std::move(pair_.second())(
            std::move(pair_.first())(std::forward<Arg>(arg)))))
            -> _invoke_result_t<self_type&&, Arg> {
        return std::move(pair_.second())(
            std::move(pair_.first())(std::forward<Arg>(arg)));
    }

    template <typename Arg>
    requires std::is_invocable_v<
                 same_cvref_t<First, const self_type&&>, Arg> &&
             std::is_invocable_v<
                 same_cvref_t<Second, const self_type&&>,
                 std::invoke_result_t<
                     same_cvref_t<First, const self_type&&>, Arg>>
    consteval auto operator()(Arg&& arg) const&&
        noexcept(noexcept(std::move(pair_.second())(
            std::move(pair_.first())(std::forward<Arg>(arg)))))
            -> _invoke_result_t<const self_type&&, Arg> {
        return std::move(pair_.second())(
            std::move(pair_.first())(std::forward<Arg>(arg)));
    }

private:
    compressed_pair<First, Second> pair_;
};

template <typename C1, typename C2>
descriptor_closure_compose(C1&&, C2&&) -> descriptor_closure_compose<
    std::remove_cvref_t<C1>, std::remove_cvref_t<C2>>;

template <world_descriptor Descriptor, descriptor_closure Closure>
consteval decltype(auto)
    operator|(Descriptor&& descriptor, Closure&& closure) noexcept {
    return std::forward<Closure>(closure)(std::forward<Descriptor>(descriptor));
}

template <descriptor_closure Closure1, descriptor_closure Closure2>
consteval decltype(auto)
    operator|(Closure1&& closure1, Closure2&& closure2) noexcept {
    return descriptor_closure_compose(
        std::forward<Closure1>(closure1), std::forward<Closure2>(closure2));
}

struct system_requirement_t {};

struct _individual_t {
    using system_requirement_concept = system_requirement_t;
};

inline constexpr _individual_t individual;

struct _always_t {
    using system_requirement_concept = system_requirement_t;
};

inline constexpr _always_t always;

template <size_t Index>
struct _group_t {
    using system_requirement_concept = system_requirement_t;
};

// default: group 0
template <size_t Index>
inline constexpr _group_t<Index> group;

template <double Freq>
struct _frequency_t {
    using system_requirement_concept = system_requirement_t;
};

/**
 * @brief Frequency of calling or scheduling.
 *
 * @tparam Freq A floating-point value indicating a time interval.
 */
template <double Freq>
constexpr _frequency_t<Freq> frequency;

} // namespace neutron
