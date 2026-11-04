#pragma once

// IWYU pragma: begin_exports

#include <version>
#if __has_include(<stdexec/stop_token.hpp>)
    #include <stop_token>
    #include <stdexec/stop_token.hpp>

namespace neutron {

using stdexec::inplace_stop_callback;
using stdexec::inplace_stop_source;
using stdexec::inplace_stop_token;
using stdexec::never_stop_token;
using std::stop_source;
using std::stop_token;
using stdexec::stoppable_token;
using stdexec::unstoppable_token;

} // namespace neutron

#elif defined(__cpp_lib_senders) && __cpp_lib_senders >= 202406L
    // The inplace and never stop tokens come with senders, which have no
    // feature-test macro of their own.
    #include <stop_token>

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

    #include <beman/execution/detail/get_stop_token.hpp>
    #include <beman/stop_token.hpp>

namespace neutron {

using beman::execution::inplace_stop_callback;
using beman::execution::inplace_stop_source;
using beman::execution::inplace_stop_token;
using beman::execution::never_stop_token;
using beman::execution::stop_source;
using beman::execution::stop_token;
using beman::execution::stoppable_token;
using beman::execution::unstoppable_token;
using beman::execution::get_stop_token;

} // namespace neutron

#else

    #error "no stop token library support"

#endif

// IWYU pragma: end_exports
