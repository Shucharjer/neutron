// IWYU pragma: private, include <neutron/type_traits.hpp>
#pragma once
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

/**
 * @brief Copy const qualifier from one type to another
 *
 * @tparam To Destination type
 * @tparam From Source type for const qualification
 */
ATOM_EXPORT template <typename To, typename From>
struct same_constness {
    using type = To;
};
template <typename To, typename From>
struct same_constness<To, From const> {
    using type = To const;
};
ATOM_EXPORT template <typename To, typename From>
using same_constness_t = typename same_constness<To, From>::type;

} // namespace neutron
