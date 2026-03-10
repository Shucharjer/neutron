#pragma once
#include <cstdint>
#include "neutron/detail/concepts/one_of.hpp"
#include "neutron/detail/metafn/specific.hpp"
#include "neutron/tstring.hpp"

namespace neutron {

enum class strategy : uint8_t {
    // state sync
    double_buffer,
    triple_buffer,
    atomic_snap,

    // stream sync
    ring_buffer,
    event_queue,
    broadcast
};

inline namespace _sync_data_scope {

struct single {};

struct in_group {};

template <tstring Name>
struct anonymous {};

struct multi {};

template <typename Ty>
concept _sync_scope = one_of<Ty, single, in_group, multi> ||
                      is_specific_value_list_v<anonymous, Ty>;

} // namespace _sync_data_scope

inline namespace _sync_data_access {

template <typename... Data>
struct input {};
template <typename... Data>
struct output {};
template <typename... Data>
struct inout {};

template <typename... Data>
struct master {};
template <typename... Data>
struct slave {};

template <typename Ty>
concept _sync_access =
    is_specific_type_list_v<input, Ty> || is_specific_type_list_v<output, Ty> ||
    is_specific_type_list_v<inout, Ty> || is_specific_type_list_v<master, Ty> ||
    is_specific_type_list_v<slave, Ty>;

} // namespace _sync_data_access

// sync should satify construct_from_world
template <_sync_scope Scope, strategy Strategy, _sync_access... Access>
requires(sizeof...(Access) != 0)
struct sync;

void sndr(sync<single, strategy::event_queue, input<int>>);
void rcvr(sync<single, strategy::event_queue, output<int>>);

void master_fn(sync<single, strategy::event_queue, master<int>>);
void slave_fn(sync<single, strategy::event_queue, slave<int>>);

} // namespace neutron
