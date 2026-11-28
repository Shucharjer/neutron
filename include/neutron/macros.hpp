#pragma once

/**
 * @file macros.hpp
 * @brief The macros of neutron, without any declaration.
 *
 * Modules cannot export macros. When neutron is used as a module
 * (`ATOM_USES_MODULES`), every public header of neutron is reduced to this
 * one, which provides the macros and imports the module.
 */

// IWYU pragma: begin_exports

#include "neutron/detail/utility/macros.hpp"

#if !ATOM_HAS_REFLECTION
    #include "neutron/detail/reflection/legacy/user_macros.hpp"
#endif

// IWYU pragma: end_exports

#if defined(ATOM_USES_MODULES) && !defined(ATOM_BUILDING_MODULE)
// GCC rejects the macro-only standard headers above after `import std;`, so
// a translation unit includes the headers of neutron before importing std.
import std;
import neutron;
#endif
