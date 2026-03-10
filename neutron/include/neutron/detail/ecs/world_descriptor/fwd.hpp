// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <cstddef>
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
struct descriptor_closure_compose :
    neutron::_closure_compose<
        descriptor_closure_compose, descriptor_adaptor_closure, First, Second> {
    using _compose_base = neutron::_closure_compose<
        descriptor_closure_compose, descriptor_adaptor_closure, First, Second>;
    using _compose_base::_compose_base;
    using _compose_base::operator();
};

template <typename C1, typename C2>
descriptor_closure_compose(C1&&, C2&&) -> descriptor_closure_compose<
    std::remove_cvref_t<C1>, std::remove_cvref_t<C2>>;

template <world_descriptor Descriptor, descriptor_closure Closure>
constexpr decltype(auto)
    operator|(Descriptor&& descriptor, Closure&& closure) noexcept {
    return std::forward<Closure>(closure)(std::forward<Descriptor>(descriptor));
}

template <descriptor_closure Closure1, descriptor_closure Closure2>
constexpr decltype(auto)
    operator|(Closure1&& closure1, Closure2&& closure2) noexcept {
    return descriptor_closure_compose(
        std::forward<Closure1>(closure1), std::forward<Closure2>(closure2));
}

struct _individual_t {};

inline constexpr _individual_t individual;

template <size_t Index>
struct _group_t {};

// default: group 0
template <size_t Index>
inline constexpr _group_t<Index> group;

template <double Freq>
struct _frequency_t {};

/**
 * @brief Frequency of calling or scheduling.
 * 
 * @tparam Freq A floating-point value indicating a time interval.
 */
template <double Freq>
constexpr _frequency_t<Freq> frequency;

} // namespace neutron
