#pragma once

#include <version>
#if defined(__cpp_lib_execution) && __cpp_lib_execution >= 202602L

    #include <execution>

namespace neutron::execution {

using namespace std::execution;

}

#elif __has_include(<stdexec/execution.hpp>)

    #include <stdexec/execution.hpp>

namespace neutron::execution {

using namespace stdexec;

}

#else

    #include "../src/neutron/internal/execution/fwd.hpp" // IWYU pragma: export

    #include "../src/neutron/internal/execution/queries.hpp" // IWYU pragma: export

    #include "../src/neutron/internal/execution/env.hpp" // IWYU pragma: export

    #include "../src/neutron/internal/execution/domain.hpp" // IWYU pragma: export

    #include "../src/neutron/internal/execution/receiver.hpp" // IWYU pragma: export

    #include "../src/neutron/internal/execution/sender.hpp" // IWYU pragma: export

    #include "../src/neutron/internal/execution/completion_signatures.hpp" // IWYU pragma: export

    #include "../src/neutron/internal/execution/connect.hpp" // IWYU pragma: export

    #include "../src/neutron/internal/execution/scheduler.hpp" // IWYU pragma: export

    #include "../src/neutron/internal/execution/sender_adaptor_closure.hpp" // IWYU pragma: export

    #include "../src/neutron/internal/execution/run_loop.hpp" // IWYU pragma: export

// sender adaptors

    #include "../src/neutron/internal/execution/sender_adaptors/then.hpp" // IWYU pragma: export

#endif
