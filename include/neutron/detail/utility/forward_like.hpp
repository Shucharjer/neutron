#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <version>
#endif
#include "neutron/detail/utility/macros.hpp"
#if defined(__cpp_lib_forward_like) && __cpp_lib_forward_like >= 202207L
    #ifndef ATOM_BUILDING_MODULE
        #include <utility>
    #endif
#else
    #include "neutron/type_traits.hpp"
#endif

namespace neutron {

#if defined(__cpp_lib_forward_like) && __cpp_lib_forward_like >= 202207L
ATOM_EXPORT using std::forward_like;
#else
ATOM_EXPORT template <typename T, typename U>
constexpr decltype(auto) forward_like(U&& u) {
    return static_cast<same_cvref_t<U, T>>(u);
}
#endif

} // namespace neutron
