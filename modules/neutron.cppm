/**
 * @file neutron.cppm
 * @brief The module interface unit of the named module `neutron`.
 *
 * The standard library and the execution backend are imported as modules,
 * and the headers of neutron are compiled in the purview with
 * `ATOM_BUILDING_MODULE` defined: their own includes of non-neutron headers
 * are skipped, and only the declarations marked with `ATOM_EXPORT` are
 * exported. The global module fragment only holds what has no module form:
 * headers for macros, platform headers and optional third-party libraries.
 */
module;

// Before any header of neutron: `ATOM_EXPORT` is defined on the first
// inclusion of the macros.
#define ATOM_BUILDING_MODULE

// Macros used by neutron, which modules do not export.
#include <cassert>
#include <cerrno>
#include <version>

#include "neutron/detail/utility/macros.hpp"

// `std::meta` is not part of the `std` module yet.
#if ATOM_HAS_REFLECTION
    #include <meta>
#endif

#if defined(ATOM_TARGET_X86)
    #include <xmmintrin.h>
#endif

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
    #if defined(__linux) || defined(__linux__)
        #include <pthread.h>
        #if ATOM_USES_NUMA && __has_include(<numa.h>)
            #include <numa.h>
        #endif
    #endif
#endif

#if __has_include(<nlohmann/json.hpp>)
    #include <nlohmann/json.hpp>
#endif
#if __has_include(<simdjson.h>)
    #include <simdjson.h>
#endif
#if __has_include(<lua.hpp>) && __has_include(<sol/sol.hpp>)
    #include <sol/sol.hpp>
#endif

export module neutron;

import std;

#if __has_include(<stdexec/execution.hpp>)
export import stdexec;
#elif __has_include(<beman/execution/execution.hpp>)
export import beman.execution;
#endif

// Generated in the build tree: every public header of neutron.
#include "neutron_all.hpp"
