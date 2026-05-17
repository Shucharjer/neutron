#pragma once
#include <thread> // IWYU pragma: export
#include <version>

namespace neutron::this_thread {

using namespace ::std::this_thread;

}

#if __has_include(<stdexec/execution.hpp>)

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

#elif defined(__cpp_lib_execution) && __cpp_lib_execution >= 202602L &&        \
    !defined(ATOM_EXECUTION)

    #include <execution> // IWYU pragma: export

namespace neutron::execution {

using namespace std::execution;

} // namespace neutron::execution

#elif __has_include(<beman/execution/execution.hpp>)

    #include <beman/execution/execution.hpp>

namespace neutron {

namespace this_thread {

using ::beman::this_thread;

}

namespace execution {

using ::beman::execution;

}

} // namespace neutron

#else

    #error "no execution control library support"

#endif
