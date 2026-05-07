#pragma once
#include <concepts>

namespace neutron {

template <typename>
struct is_convertible_to_function_pointer {
    static constexpr bool value = false;
};

template <typename Ret, typename... Args>
struct is_convertible_to_function_pointer<Ret (*)(Args...)> {
    static constexpr bool value = true;
};

template <typename Ty>
requires std::is_class_v<Ty>
struct is_convertible_to_function_pointer<Ty> {
    template <typename>
    static constexpr bool _impl = false;

    static constexpr bool value = requires {
        &Ty::operator();
        _impl<decltype(&Ty::operator())>;
    };

    template <typename Ret, typename... Args>
    static constexpr bool _impl<Ret (Ty::*)(Args...)> =
        requires(const Ty& val) {
            { val } -> std::convertible_to<Ret (*)(Args...)>;
        };

    template <typename Ret, typename... Args>
    static constexpr bool _impl<Ret (Ty::*)(Args...) const> =
        requires(const Ty& val) {
            { val } -> std::convertible_to<Ret (*)(Args...)>;
        };

    template <typename Ret, typename... Args>
    static constexpr bool _impl<Ret (Ty::*)(Args...) noexcept> =
        requires(const Ty& val) {
            { val } -> std::convertible_to<Ret (*)(Args...) noexcept>;
        };

    template <typename Ret, typename... Args>
    static constexpr bool _impl<Ret (Ty::*)(Args...) const noexcept> =
        requires(const Ty& val) {
            { val } -> std::convertible_to<Ret (*)(Args...) noexcept>;
        };
};

template <typename Ty>
constexpr bool is_convertible_to_function_pointer_v =
    is_convertible_to_function_pointer<Ty>::value;

} // namespace neutron
