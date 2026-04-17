#pragma once
#include <tuple>

namespace neutron {

template <typename T>
concept tuple_like = requires {
    typename std::tuple_element_t<0, T>;
    typename std::tuple_size<T>;
};

} // namespace neutron
