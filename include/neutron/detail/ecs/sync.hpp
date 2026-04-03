// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <neutron/ecs.hpp>
#include "neutron/detail/concepts/one_of.hpp"
#include "neutron/detail/metafn/cat.hpp"
#include "neutron/detail/metafn/definition.hpp"
#include "neutron/detail/metafn/has.hpp"
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

enum expire : uint8_t {
    frame,
    full,
    never
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
struct input {
    using access_types = type_list<Data...>;
    using data_types   = type_list<std::remove_cvref_t<Data>...>;
};
template <typename... Data>
struct output {
    using access_types = type_list<Data...>;
    using data_types   = type_list<std::remove_cvref_t<Data>...>;
};
template <typename... Data>
struct inout {
    using access_types = type_list<Data...>;
    using data_types   = type_list<std::remove_cvref_t<Data>...>;
};

template <typename... Data>
struct master {
    using access_types = type_list<Data...>;
    using data_types   = type_list<std::remove_cvref_t<Data>...>;
};
template <typename... Data>
struct slave {
    using access_types = type_list<Data...>;
    using data_types   = type_list<std::remove_cvref_t<Data>...>;
};

template <typename Ty>
concept _sync_access =
    is_specific_type_list_v<input, Ty> || is_specific_type_list_v<output, Ty> ||
    is_specific_type_list_v<inout, Ty> || is_specific_type_list_v<master, Ty> ||
    is_specific_type_list_v<slave, Ty>;

} // namespace _sync_data_access

namespace _sync {

template <_sync_scope Scope>
class _sync_storage;

template <>
class _sync_storage<single> {
    //
};

template <>
class _sync_storage<in_group> {
    //
};

template <>
class _sync_storage<multi> {
    //
};

template <strategy Strategy, _sync_access Access>
class _sync_access;

template <
    strategy Strategy, template <typename...> typename Accessibility,
    typename Ty>
class _sync_accessor;

template <
    strategy Strategy, template <typename...> typename Access, typename... Tys>
class _sync_access<Strategy, Access<Tys...>> :
    public std::tuple<_sync_accessor<Strategy, Access, Tys>...> {
public:
    using std::tuple<_sync_accessor<Strategy, Access, Tys>...>::tuple;

private:
};

template <typename Ty>
class _sync_accessor<strategy::atomic_snap, output, Ty> {

public:
    template <typename... Args>
    requires std::constructible_from<Ty, Args...>
    void write(Args&&..., std::memory_order = std::memory_order::seq_cst) {}

private:
};

template <typename Ty>
class _sync_accessor<strategy::atomic_snap, input, Ty> {
public:
    Ty read(std::memory_order = std::memory_order::seq_cst) {}

private:
};

template <typename Ty>
class _sync_accessor<strategy::atomic_snap, inout, Ty> {
public:
    Ty read(std::memory_order = std::memory_order::seq_cst) {}

    template <typename... Args>
    requires std::constructible_from<Ty, Args...>
    void write(Args&&..., std::memory_order = std::memory_order::seq_cst) {}

private:
};

template <typename Ty>
class _sync_accessor<strategy::double_buffer, output, Ty> {
public:
    template <typename... Args>
    requires std::constructible_from<Ty, Args...>
    void set(Args&&..., std::memory_order = std::memory_order::seq_cst);
};
template <typename Ty>
class _sync_accessor<strategy::double_buffer, input, Ty> {
public:
    Ty use(std::memory_order = std::memory_order::seq_cst);
};

template <typename Ty>
class _sync_accessor<strategy::triple_buffer, output, Ty> {
public:
    template <typename... Args>
    requires std::constructible_from<Ty, Args...>
    void set(Args&&..., std::memory_order = std::memory_order::seq_cst);
};
template <typename Ty>
class _sync_accessor<strategy::triple_buffer, input, Ty> {
public:
    Ty use(std::memory_order = std::memory_order::seq_cst);
};

template <typename Ty>
class _sync_accessor<strategy::ring_buffer, output, Ty> {
public:
    template <typename... Args>
    requires std::constructible_from<Ty, Args...>
    void set(Args&&..., std::memory_order = std::memory_order::seq_cst);
};
template <typename Ty>
class _sync_accessor<strategy::ring_buffer, input, Ty> {
public:
    Ty use(std::memory_order = std::memory_order::seq_cst);
};

template <typename Ty>
class _sync_accessor<strategy::broadcast, output, Ty> {
public:
    template <typename... Args>
    requires std::constructible_from<Ty, Args...>
    void broadcast(Args&&...);
};
template <typename Ty>
class _sync_accessor<strategy::broadcast, input, Ty> {
public:
    Ty recv();
};

template <typename Ty>
class _sync_accessor<strategy::broadcast, master, Ty> {
public:
    template <typename... Args>
    requires std::constructible_from<Ty, Args...>
    void broadcast(Args&&...);
};
template <typename Ty>
class _sync_accessor<strategy::broadcast, slave, Ty> {
public:
    Ty recv();
};

template <typename Ty>
class _sync_accessor<strategy::event_queue, input, Ty> {
public:
    template <typename... Args>
    requires std::constructible_from<Ty, Args...>
    void push(Args&&... args, expire exp = expire::frame);
};
template <typename Ty>
class _sync_accessor<strategy::event_queue, output, Ty> {
public:
    Ty pop();
};

} // namespace _sync

// sync should satify construct_from_world
template <_sync_scope Scope, strategy Strategy, _sync_access... Access>
requires(sizeof...(Access) != 0)
class sync :
    private _sync::_sync_storage<Scope>,
    public std::tuple<_sync::_sync_access<Strategy, Access>...> {
public:
    using access_types = type_list_cat_t<typename Access::access_types...>;
    using data_types   = type_list_cat_t<typename Access::data_types...>;

    using std::tuple<_sync::_sync_access<Strategy, Access>...>::tuple;

    sync(const sync&)            = delete;
    sync& operator=(const sync&) = delete;
    sync(sync&&)                 = delete;
    sync& operator=(sync&&)      = delete;
    ~sync()                      = default;
};

template <
    auto Sys, _sync_scope Scope, strategy Strategy, _sync_access... Access,
    size_t Index>
struct construct_from_world_t<Sys, sync<Scope, Strategy, Access...>, Index> {
    template <world World>
    auto operator()(World& world) const -> sync<Scope, Strategy, Access...> {
        // using group_id = World::group_id;
        return {};
    }
};

} // namespace neutron

namespace std {

template <
    neutron::_sync_scope Scope, neutron::strategy Strategy,
    neutron::_sync_access... Access>
struct tuple_size<neutron::sync<Scope, Strategy, Access...>> {
    static constexpr size_t value = sizeof...(Access);
};

template <
    size_t Index, neutron::_sync_scope Scope, neutron::strategy Strategy,
    neutron::_sync_access... Access>
struct tuple_element<Index, neutron::sync<Scope, Strategy, Access...>> {
    using type = std::tuple_element_t<
        Index, std::tuple<neutron::_sync::_sync_access<Strategy, Access>...>>;
};

template <::neutron::strategy Strategy, ::neutron::_sync_access Access>
struct tuple_size<::neutron::_sync::_sync_access<Strategy, Access>> {
    static constexpr size_t value = ::neutron::type_list_size_v<Access>;
};

template <
    size_t Index, ::neutron::strategy Strategy,
    template <typename...> typename Access, typename... Tys>
struct tuple_element<
    Index, ::neutron::_sync::_sync_access<Strategy, Access<Tys...>>> {
    using type = typename ::neutron::type_list_element_t<
        Index,
        std::tuple<::neutron::_sync::_sync_accessor<Strategy, Access, Tys>...>>;
};

} // namespace std
