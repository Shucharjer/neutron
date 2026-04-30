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
    #define ATOM_BUILTIN_EXECUTION
// IWYU pragma: begin_exports

    #include "neutron/detail/execution/fwd.hpp" // IWYU pragma: export

    #include "neutron/detail/execution/queries.hpp"
    #include "neutron/detail/execution/queries/get_await_completion_adaptor.hpp"
    #include "neutron/detail/execution/queries/get_completion_scheduler.hpp"
    #include "neutron/detail/execution/queries/get_delegation_scheduler.hpp"
    #include "neutron/detail/execution/queries/get_domain.hpp"
    #include "neutron/detail/execution/queries/get_env.hpp"
    #include "neutron/detail/execution/queries/get_scheduler.hpp"

    #include "neutron/detail/execution/receivers/set_error.hpp"
    #include "neutron/detail/execution/receivers/set_stopped.hpp"
    #include "neutron/detail/execution/receivers/set_value.hpp"

    #include "neutron/detail/execution/operation_states/start.hpp"

    #include "neutron/detail/execution/senders/apply_sender.hpp"
    #include "neutron/detail/execution/senders/connect.hpp"
    #include "neutron/detail/execution/senders/default_domain.hpp"
    #include "neutron/detail/execution/senders/get_completion_signatures.hpp"
    #include "neutron/detail/execution/senders/transform_sender.hpp"
    // sender factories
    #include "neutron/detail/execution/senders/sender_factories/just.hpp"
    #include "neutron/detail/execution/senders/sender_factories/just_error.hpp"
    #include "neutron/detail/execution/senders/sender_factories/just_stopped.hpp"
    #include "neutron/detail/execution/senders/sender_factories/schedule.hpp"
    // sender adaptors
    #include "neutron/detail/execution/senders/sender_adaptors/bulk.hpp"
    #include "neutron/detail/execution/senders/sender_adaptors/bulk_chunked.hpp"
    #include "neutron/detail/execution/senders/sender_adaptors/bulk_unchunked.hpp"
    #include "neutron/detail/execution/senders/sender_adaptors/continues_on.hpp"
    #include "neutron/detail/execution/senders/sender_adaptors/let_value.hpp"
    #include "neutron/detail/execution/senders/sender_adaptors/schedule_from.hpp"
    #include "neutron/detail/execution/senders/sender_adaptors/then.hpp"
    #include "neutron/detail/execution/senders/sender_adaptors/when_all.hpp"
    // sender consumers
    #include "neutron/detail/execution/senders/sender_consumers/sync_wait.hpp"

    #include "neutron/detail/execution/queryable_utilities/env.hpp"

    #include "neutron/detail/execution/execution_contexts/run_loop.hpp"

    #include "neutron/detail/execution/coroutine_utilities/affine.hpp"
    #include "neutron/detail/execution/coroutine_utilities/as_awaitable.hpp"
    #include "neutron/detail/execution/coroutine_utilities/inline_scheduler.hpp"
    #include "neutron/detail/execution/coroutine_utilities/task.hpp"
    #include "neutron/detail/execution/coroutine_utilities/task_scheduler.hpp"
    #include "neutron/detail/execution/coroutine_utilities/with_awaitable_senders.hpp"

    #include "neutron/detail/execution/parallel_scheduler_replacement/parallel_scheduler_backend.hpp"
    #include "neutron/detail/execution/parallel_scheduler_replacement/receiver_proxy.hpp"

// IWYU pragma: end_exports

#endif
