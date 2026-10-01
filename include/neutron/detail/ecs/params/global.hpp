// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include "neutron/detail/ecs/fwd.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron {

template <typename T>
struct _glob_storage {
    static T& instance() {
        static T inst;
        return inst;
    }
};

template <typename... Methods>
class global : public std::tuple<Methods&...> {
public:
    global() : std::tuple<Methods&...>(_glob_storage<Methods>::instance()...) {}
};

namespace internal {

template <typename>
struct _is_global : std::false_type {};

template <typename... Args>
struct _is_global<global<Args...>> : std::true_type {};

} // namespace internal

} // namespace neutron

template <typename... Args>
struct std::tuple_size<::neutron::global<Args...>> :
    integral_constant<size_t, sizeof...(Args)> {};

template <size_t Index, typename... Args>
struct std::tuple_element<Index, neutron::global<Args...>> {
    using type = tuple_element_t<Index, tuple<Args&...>>;
};
