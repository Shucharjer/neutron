#pragma once
#include "neutron/template_list.hpp"

namespace neutron {

template <typename>
struct member_function_traits;
template <typename Ret, typename Class, typename... Args>
struct member_function_traits<Ret (Class::*)(Args...)> {
    using type                        = Ret (Class::*)(Args...);
    using function_type               = Ret(Args...);
    using static_type                 = Ret (*)(Class&, Args...);
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
    using function_type               = Ret(Args...);
    using static_type                 = Ret (*)(const Class&, Args...);
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
    using function_type               = Ret(Args...) noexcept;
    using static_type                 = Ret (*)(Class&, Args...) noexcept;
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
    using function_type               = Ret(Args...) noexcept;
    using static_type                 = Ret (*)(const Class&, Args...) noexcept;
    using void_type                   = Ret (*)(const void*, Args...) noexcept;
    using erased_type                 = Ret (*)(Args...) noexcept;
    using return_type                 = Ret;
    using class_type                  = Class;
    using args_type                   = type_list<Args...>;
    constexpr static bool is_noexcept = true;
    constexpr static size_t num_args  = sizeof...(Args);
};

template <auto Fn, typename FnTy = decltype(Fn)>
struct member_fn;
template <auto Fn, typename Ret, typename Class, typename... Args>
struct member_fn<Fn, Ret (Class::*)(Args...) const> {
    constexpr static auto static_fn =
        [](const Class& obj, Args... args) -> Ret { (obj.*Fn)(args...); };
    constexpr static auto void_fn = [](const void* ptr, Args... args) -> Ret {
        return (static_cast<const Class*>(ptr)->*Fn)(args...);
    };
};
template <auto Fn, typename Ret, typename Class, typename... Args>
struct member_fn<Fn, Ret (Class::*)(Args...) const noexcept> {
    constexpr static auto static_fn = [](const Class& obj,
                                         Args... args) noexcept -> Ret {
        (obj.*Fn)(args...);
    };
    constexpr static auto void_fn = [](const void* ptr,
                                       Args... args) noexcept -> Ret {
        return (static_cast<const Class*>(ptr)->*Fn)(args...);
    };
};
template <auto Fn, typename Ret, typename Class, typename... Args>
struct member_fn<Fn, Ret (Class::*)(Args...)> {
    constexpr static auto static_fn = [](Class& obj, Args... args) -> Ret {
        (obj.*Fn)(args...);
    };
    constexpr static auto void_fn = [](void* ptr, Args... args) -> Ret {
        return (static_cast<Class*>(ptr)->*Fn)(args...);
    };
};
template <auto Fn, typename Ret, typename Class, typename... Args>
struct member_fn<Fn, Ret (Class::*)(Args...) noexcept> {
    constexpr static auto static_fn =
        [](Class& obj, Args... args) noexcept -> Ret { (obj.*Fn)(args...); };
    constexpr static auto void_fn = [](void* ptr,
                                       Args... args) noexcept -> Ret {
        return (static_cast<Class*>(ptr)->*Fn)(args...);
    };
};

} // namespace neutron
