// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#include <type_traits>

namespace neutron {

template <typename T, template <typename...> typename Template>
struct _is_instance_of_impl : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct _is_instance_of_impl<Template<Args...>, Template> : std::true_type {};

template <typename T, template <typename...> typename... Templates>
concept instance_of = (_is_instance_of_impl<T, Templates>::value || ...);

} // namespace neutron
