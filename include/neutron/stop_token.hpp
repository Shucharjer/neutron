#pragma once

#if defined(ATOM_USES_MODULES) && !defined(ATOM_BUILDING_MODULE)
    #include "neutron/macros.hpp"
#else

    #include "neutron/detail/utility/macros.hpp"

// IWYU pragma: begin_exports

    #ifndef ATOM_BUILDING_MODULE
        #include <version>
    #endif
    #if __has_include(<stdexec/stop_token.hpp>)
        #ifndef ATOM_BUILDING_MODULE
            #include <stop_token>
            #include <stdexec/stop_token.hpp>
        #endif

namespace neutron {

ATOM_EXPORT using stdexec::inplace_stop_callback;
ATOM_EXPORT using stdexec::inplace_stop_source;
ATOM_EXPORT using stdexec::inplace_stop_token;
ATOM_EXPORT using stdexec::never_stop_token;
ATOM_EXPORT using std::stop_source;
ATOM_EXPORT using std::stop_token;
ATOM_EXPORT using stdexec::stoppable_token;
ATOM_EXPORT using stdexec::unstoppable_token;

} // namespace neutron

    #elif defined(__cpp_lib_senders) && __cpp_lib_senders >= 202406L
        // The inplace and never stop tokens come with senders, which have no
        // feature-test macro of their own.
        #ifndef ATOM_BUILDING_MODULE
            #include <stop_token>
        #endif

namespace neutron {

using std::get_stop_token;
using std::inplace_stop_callback;
using std::inplace_stop_source;
using std::inplace_stop_token;
using std::never_stop_token;
using std::stop_source;
using std::stop_token;
using std::stoppable_token;
using std::unstoppable_token;

} // namespace neutron

    #elif __has_include(<beman/stop_token.hpp>)

        #ifndef ATOM_BUILDING_MODULE
            #include <beman/execution/detail/get_stop_token.hpp>
            #include <beman/stop_token.hpp>
        #endif

namespace neutron {

ATOM_EXPORT using beman::execution::inplace_stop_callback;
ATOM_EXPORT using beman::execution::inplace_stop_source;
ATOM_EXPORT using beman::execution::inplace_stop_token;
ATOM_EXPORT using beman::execution::never_stop_token;
ATOM_EXPORT using beman::execution::stop_source;
ATOM_EXPORT using beman::execution::stop_token;
ATOM_EXPORT using beman::execution::stoppable_token;
ATOM_EXPORT using beman::execution::unstoppable_token;
ATOM_EXPORT using beman::execution::get_stop_token;

} // namespace neutron

    #else

        #error "no stop token library support"

    #endif

// IWYU pragma: end_exports

#endif
