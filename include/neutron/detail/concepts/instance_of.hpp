// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once

namespace neutron {

template <typename T, template <typename...> typename Template>
constexpr bool _is_instance_of = false;

template <template <typename...> typename Template, typename... Args>
constexpr bool _is_instance_of<Template<Args...>, Template> = true;

template <typename T, template <typename...> typename... Templates>
concept instance_of = (_is_instance_of<T, Templates> || ...);

} // namespace neutron
