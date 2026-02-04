#pragma once
#include <thread> // IWYU pragma: export

namespace neutron::this_thread {

using namespace ::std::this_thread;

}

#include <version>
#if defined(__cpp_lib_execution) && __cpp_lib_execution >= 202602L
    #define NEUTRON_EXECUTION 1

    #include <execution> // IWYU pragma: export

namespace neutron::execution {

using namespace std::execution;

} // namespace neutron::execution

#elif __has_include(<stdexec/execution.hpp>)
    #define NEUTRON_EXECUTION 2

    #include <stdexec/execution.hpp>

namespace neutron {

namespace execution {

using namespace stdexec;

struct scheduler_t {};

} // namespace execution

namespace this_thread {

using ::stdexec::sync_wait;

}

} // namespace neutron

#else
    #define NEUTRON_EXECUTION 0

    #include "neutron/detail/execution/fwd.hpp"      // IWYU pragma: export

    #include "neutron/detail/execution/queries.hpp"  // IWYU pragma: export

    #include "neutron/detail/execution/env.hpp"      // IWYU pragma: export

    #include "neutron/detail/execution/domain.hpp"   // IWYU pragma: export

    #include "neutron/detail/execution/receiver.hpp" // IWYU pragma: export

    #include "neutron/detail/execution/sender.hpp"   // IWYU pragma: export

    #include "neutron/detail/execution/completion_signatures.hpp" // IWYU pragma : export

    #include "neutron/detail/execution/connect.hpp"   // IWYU pragma: export

    #include "neutron/detail/execution/scheduler.hpp" // IWYU pragma: export

    #include "neutron/detail/execution/sender_adaptor_closure.hpp" // IWYU pragma: export

    #include "neutron/detail/execution/run_loop.hpp" // IWYU pragma: export

// sender consumers

    #include "neutron/detail/execution/sender_consumers/sync_wait.hpp" // IWYU pragma: export

// sender adaptors

    #include "neutron/detail/execution/sender_adaptors/then.hpp" // IWYU pragma: export

#endif
