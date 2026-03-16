#pragma once
#include <version>
#if defined(__cpp_lib_forward_like) && __cpp_lib_forward_like >= 202207L
    #include <utility>
#else
    #include <neutron/type_traits.hpp>
#endif

namespace neutron {

#if defined(__cpp_lib_forward_like) && __cpp_lib_forward_like >= 202207L
using std::forward_like;
#else
template <typename T, typename U>
constexpr decltype(auto) forward_like(U&& u) {
    return static_cast<same_cvref_t<U, T>>(u);
}
#endif

} // namespace neutron
