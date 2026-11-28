#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <thread> // IWYU pragma: export
    #include <version>
#endif
#include "neutron/detail/utility/macros.hpp"

// Exported as a whole: a using-directive only reaches the importers through
// an exported namespace definition.
ATOM_EXPORT namespace neutron::this_thread {

    using namespace ::std::this_thread;
}

#if __has_include(<stdexec/execution.hpp>)

    #ifndef ATOM_BUILDING_MODULE
        #include <stdexec/execution.hpp>
    #endif

namespace neutron {

ATOM_EXPORT using ::stdexec::get_stop_token;

ATOM_EXPORT namespace execution {

    using namespace stdexec;

    struct scheduler_t {};

} // namespace execution

namespace this_thread {

ATOM_EXPORT using ::stdexec::sync_wait;
}

} // namespace neutron

#elif defined(__cpp_lib_senders) && __cpp_lib_senders >= 202406L &&            \
    !defined(ATOM_EXECUTION)

    #ifndef ATOM_BUILDING_MODULE
        #include <execution>  // IWYU pragma: export
        #include <stop_token> // IWYU pragma: export
    #endif

namespace neutron {

ATOM_EXPORT using ::std::get_stop_token;

ATOM_EXPORT namespace execution {

    using namespace ::std::execution;

    // Declared in namespace std rather than std::execution.
    using ::std::forwarding_query;
    using ::std::get_allocator;
    using ::std::get_stop_token;
    using ::std::get_stop_token_t;
    using ::std::inplace_stop_callback;
    using ::std::inplace_stop_source;
    using ::std::inplace_stop_token;
    using ::std::never_stop_token;
    using ::std::stop_source;
    using ::std::stop_token;
    using ::std::stoppable_token;
    using ::std::unstoppable_token;

} // namespace execution

} // namespace neutron

#elif __has_include(<beman/execution/execution.hpp>)

    #ifndef ATOM_BUILDING_MODULE
        #include <beman/execution/execution.hpp>
    #endif

namespace neutron {

ATOM_EXPORT using ::beman::execution::get_stop_token;

namespace this_thread {

ATOM_EXPORT using ::beman::execution::sync_wait;
}

ATOM_EXPORT namespace execution { using namespace ::beman::execution; }

} // namespace neutron

#else

    #error "no execution control library support"

#endif
