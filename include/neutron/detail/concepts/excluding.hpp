// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#include <concepts>
#include <type_traits>

namespace neutron {

template <typename T, typename... U>
concept excluding = (!std::same_as<std::remove_cvref_t<T>, U> && ...);

}
