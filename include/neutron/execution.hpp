#pragma once
#include <thread> // IWYU pragma: export

namespace neutron::this_thread {

using namespace ::std::this_thread;

}

#include <version>
#if defined(__cpp_lib_execution) && __cpp_lib_execution >= 202602L &&          \
    !defined(ATOM_EXECUTION)

    #include <execution> // IWYU pragma: export

namespace neutron::execution {

using namespace std::execution;

} // namespace neutron::execution

#elif __has_include(<stdexec/execution.hpp>) && !defined(ATOM_EXECUTION)

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
// IWYU pragma: begin_exports

    #include "neutron/detail/execution/fwd.hpp" // IWYU pragma: export

    #include "neutron/detail/execution/connect.hpp"
    #include "neutron/detail/execution/default_domain.hpp"
    #include "neutron/detail/execution/fwd_env.hpp"
    #include "neutron/detail/execution/get_completion_scheduler.hpp"
    #include "neutron/detail/execution/get_completion_signatures.hpp"
    #include "neutron/detail/execution/get_domain.hpp"
    #include "neutron/detail/execution/get_scheduler.hpp"
    #include "neutron/detail/execution/make_sender.hpp"
    #include "neutron/detail/execution/product_type.hpp"
    #include "neutron/detail/execution/queries.hpp"
    #include "neutron/detail/execution/query_with_default.hpp"
    #include "neutron/detail/execution/run_loop.hpp"
    #include "neutron/detail/execution/sender_adaptor.hpp"
    #include "neutron/detail/execution/sender_adaptor_closure.hpp"
    #include "neutron/detail/execution/set_error.hpp"
    #include "neutron/detail/execution/set_stopped.hpp"
    #include "neutron/detail/execution/set_value.hpp"
    #include "neutron/detail/execution/start.hpp"

// sender factories

    #include "neutron/detail/execution/sender_factories/just.hpp"
    #include "neutron/detail/execution/sender_factories/just_error.hpp"
    #include "neutron/detail/execution/sender_factories/just_stopped.hpp"
    #include "neutron/detail/execution/sender_factories/schedule.hpp"

// sender consumers

    #include "neutron/detail/execution/sender_consumers/sync_wait.hpp"

// sender adaptors

    #include "neutron/detail/execution/sender_adaptors/bulk.hpp"
    #include "neutron/detail/execution/sender_adaptors/continues_on.hpp"
    #include "neutron/detail/execution/sender_adaptors/then.hpp"
// TODO: finish when_all
// #include "neutron/detail/execution/sender_adaptors/when_all.hpp"

#endif
