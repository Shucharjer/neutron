// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <cstddef>
#include <tuple>
#include <type_traits>
#include "neutron/detail/ecs/construct_from_world.hpp"
#include "neutron/detail/ecs/systuple.hpp"
#include "neutron/detail/ecs/world_accessor.hpp"
#include "neutron/detail/tuple/first.hpp"

namespace neutron {

template <typename... Args>
class local {
public:
    static_assert((std::is_same_v<std::remove_const_t<Args>, Args> && ...));

    template <auto Sys>
    constexpr local(systuple<Sys, Args...>& tup) noexcept : tup_(tup) {}

    template <size_t Index>
    constexpr decltype(auto) get() noexcept {
        return std::get<Index>(tup_);
    }

    template <size_t Index>
    constexpr decltype(auto) get() const noexcept {
        return std::get<Index>(tup_);
    }

private:
    std::tuple<Args...>& tup_; // NOLINT
};

template <auto Sys, size_t Index, typename... Args>
struct construct_from_world_t<Sys, local<Args...>, Index> {
    template <typename Ty>
    using _predicate = internal::_is_relevant_sys_tuple<Sys, Ty>;

    template <world World>
    local<Args...> operator()(World& world) const noexcept {
        using sys_tuple = type_list_first_t<
            type_list_filt_t<_predicate, typename World::locals>>;
        auto& locals = world_accessor::locals(world);
        return local<Args...>(neutron::get_first<sys_tuple>(locals));
    }
};

namespace internal {

template <typename>
struct _is_local : std::false_type {};
template <typename... Locals>
struct _is_local<local<Locals...>> : std::true_type {};

} // namespace internal

} // namespace neutron

template <typename... Args>
struct std::tuple_size<neutron::local<Args...>> :
    std::integral_constant<size_t, sizeof...(Args)> {};

template <size_t Index, typename... Args>
struct std::tuple_element<Index, neutron::local<Args...>> {
    using type = std::tuple_element_t<Index, std::tuple<Args&...>>;
};
