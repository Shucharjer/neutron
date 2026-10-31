#pragma once

// IWYU pragma: begin_exports

#include <version>
#if __has_include(<stdexec/stop_token.hpp>)
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

#elif defined(__cpp_lib_stop_token) && __cpp_lib_stop_token >= 202602L

namespace neutron {

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

    #include <beman/stop_token.hpp>
    #include <beman/execution/detail/get_stop_token.hpp>

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

#endif

// IWYU pragma: end_exports
