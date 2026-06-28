#pragma once
#include <type_traits>

namespace neutron {

namespace _concepts {

template <typename T>
struct nonempty : std::negation<std::is_empty<T>> {};

} // namespace _concepts

template <typename T>
concept nonempty = _concepts::nonempty<T>::value;

} // namespace neutron
