#pragma once
#include <thread> // IWYU pragma: export
#include <version>

namespace neutron::this_thread {

using namespace ::std::this_thread;

}

#if __has_include(<stdexec/execution.hpp>)

    #include <stdexec/execution.hpp>

namespace neutron {

using ::stdexec::get_stop_token;

namespace execution {

using namespace stdexec;

struct scheduler_t {};

} // namespace execution

namespace this_thread {

using ::stdexec::sync_wait;

}

} // namespace neutron

#elif defined(__cpp_lib_senders) && __cpp_lib_senders >= 202406L &&            \
    !defined(ATOM_EXECUTION)

    #include <execution>  // IWYU pragma: export
    #include <stop_token> // IWYU pragma: export

namespace neutron {

using ::std::get_stop_token;

namespace execution {

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

    #include <beman/execution/execution.hpp>

namespace neutron {

using ::beman::execution::get_stop_token;

namespace this_thread {

using ::beman::execution::sync_wait;

}

namespace execution {

using namespace ::beman::execution;

}

} // namespace neutron

#else

    #error "no execution control library support"

#endif
