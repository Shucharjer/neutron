#pragma once

#if defined(ATOM_USES_MODULES) && !defined(ATOM_BUILDING_MODULE)
    #include "neutron/macros.hpp"
#else

    #include "neutron/detail/utility/macros.hpp"

namespace neutron::constants {

namespace integral {} // namespace integral

namespace floating {

ATOM_EXPORT inline constexpr auto _1_2   = 0.5F;
ATOM_EXPORT inline constexpr auto _1_2_d = 0.5;

} // namespace floating

} // namespace neutron::constants

#endif
