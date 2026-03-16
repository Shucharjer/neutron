#pragma once
#include <cstddef>
#include <tuple>
#include <type_traits>
#include "neutron/detail/ecs/resource.hpp"
#include "neutron/detail/ecs/world_accessor.hpp"
#include "neutron/detail/tuple/rmcvref_first.hpp"

namespace neutron {

template <resource_like... Resources>
struct res : public std::tuple<Resources...> {
    template <world World>
    res(World& world)
        : std::tuple<Resources...>(
              neutron::rmcvref_first<std::remove_cvref_t<Resources>>(
                  world_accessor::resources(world))...) {}
};

namespace internal {

template <typename>
struct _is_res : std::false_type {};
template <resource_like... Resources>
struct _is_res<res<Resources...>> : std::true_type {};

} // namespace internal

} // namespace neutron

template <typename... Resources>
struct std::tuple_size<neutron::res<Resources...>> :
    std::integral_constant<size_t, sizeof...(Resources)> {};

template <size_t Index, neutron::resource_like... Resources>
struct std::tuple_element<Index, neutron::res<Resources...>> {
    using type = std::tuple_element_t<Index, std::tuple<Resources&...>>;
};
