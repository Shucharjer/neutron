// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <type_traits>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

template <typename T, template <typename...> typename Template>
struct _is_instance_of_impl : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct _is_instance_of_impl<Template<Args...>, Template> : std::true_type {};

ATOM_EXPORT template <typename T, template <typename...> typename... Templates>
concept instance_of = (_is_instance_of_impl<T, Templates>::value || ...);

} // namespace neutron
