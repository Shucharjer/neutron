#pragma once
#include <concepts>
#include <cstdint>

namespace neutron {

template <typename Ty>
concept queryable = std::destructible<Ty>;

// [exec.queries], queries
struct forwarding_query_t;
struct get_allocator_t;
struct get_stop_token_t;

namespace execution {

// [exec.queries], queries
enum class forward_progress_guarantee : uint8_t {
    concurrent,
    parallel,
    weakly_parallel
};

struct empty_env {};

// [exec.domain.default], execution domains
struct default_domain;

// [exec.sched], schedulers
struct scheduler_t;

// [exec.recv], receivers
struct receiver_t;

struct set_value_t;
struct set_error_t;
struct set_stopped_t;

// [exec.opstate], operation states
struct operation_state_t;

struct start_t;

struct sender_t;

struct get_completion_signatures_t;

struct connect_t;

// [exec.factories], sender factories
struct just_t;
struct just_error_t;
struct just_stopped_t;
struct schedule_t;

// [exec.adapt], sender adaptors
template <typename Derived>
struct sender_adaptor_closure;

struct starts_on_t;
struct continues_on_t;
struct on_t;
struct schedule_from_t;
struct then_t;
struct upon_error_t;
struct upon_stopped_t;
struct let_value_t;
struct let_error_t;
struct let_stopped_t;
struct bulk_t;
struct split_t;
struct when_all_t;
struct when_all_with_variant_t;
struct into_variant_t;
struct stopped_as_optional_t;
struct stopped_as_error_t;

class run_loop;

} // namespace execution

} // namespace neutron
