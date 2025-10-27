#pragma once
#include "neutron/type_list.hpp"

namespace neutron {

template <typename>
struct member_function_traits;
template <typename Ret, typename Class, typename... Args>
struct member_function_traits<Ret (Class::*)(Args...)> {
    using type                        = Ret (Class::*)(Args...);
    using static_type                 = Ret (*)(void*, Args...);
    using void_type                   = Ret (*)(void*, Args...);
    using erased_type                 = Ret (*)(Args...);
    using return_type                 = Ret;
    using class_type                  = Class;
    using args_type                   = type_list<Args...>;
    constexpr static bool is_noexcept = false;
    constexpr static size_t num_args  = sizeof...(Args);
};
template <typename Ret, typename Class, typename... Args>
struct member_function_traits<Ret (Class::*)(Args...) const> {
    using type                        = Ret (Class::*)(Args...) const;
    using static_type                 = Ret (*)(const void*, Args...);
    using void_type                   = Ret (*)(const void*, Args...);
    using erased_type                 = Ret (*)(Args...);
    using return_type                 = Ret;
    using class_type                  = Class;
    using args_type                   = type_list<Args...>;
    constexpr static bool is_noexcept = false;
    constexpr static size_t num_args  = sizeof...(Args);
};
template <typename Ret, typename Class, typename... Args>
struct member_function_traits<Ret (Class::*)(Args...) noexcept> {
    using type                        = Ret (Class::*)(Args...) noexcept;
    using static_type                 = Ret (*)(void*, Args...) noexcept;
    using void_type                   = Ret (*)(void*, Args...) noexcept;
    using erased_type                 = Ret (*)(Args...) noexcept;
    using return_type                 = Ret;
    using class_type                  = Class;
    using args_type                   = type_list<Args...>;
    constexpr static bool is_noexcept = true;
    constexpr static size_t num_args  = sizeof...(Args);
};
template <typename Ret, typename Class, typename... Args>
struct member_function_traits<Ret (Class::*)(Args...) const noexcept> {
    using type                        = Ret (Class::*)(Args...) const noexcept;
    using static_type                 = Ret (*)(const void*, Args...) noexcept;
    using void_type                   = Ret (*)(const void*, Args...) noexcept;
    using erased_type                 = Ret (*)(Args...) noexcept;
    using return_type                 = Ret;
    using class_type                  = Class;
    using args_type                   = type_list<Args...>;
    constexpr static bool is_noexcept = true;
    constexpr static size_t num_args  = sizeof...(Args);
};

} // namespace neutron
