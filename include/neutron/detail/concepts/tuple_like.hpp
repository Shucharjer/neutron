#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <tuple>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename T>
concept tuple_like = requires {
    typename std::tuple_element_t<0, T>;
    typename std::tuple_size<T>;
};

} // namespace neutron
