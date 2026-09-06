// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#include <concepts>

namespace neutron {

template <typename T, typename... Others>
concept same = (std::same_as<T, Others> && ...);

}
