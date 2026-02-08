#pragma once
#include <concepts>
#include <type_traits>

namespace neutron {

template <typename Ty>
concept movable_value = std::move_constructible<Ty> &&
                        std::constructible_from<std::decay_t<Ty>, Ty>;

}
