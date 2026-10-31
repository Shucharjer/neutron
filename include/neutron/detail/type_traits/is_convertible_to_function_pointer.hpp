#pragma once
#include <concepts>
#include <type_traits>

namespace neutron {

template <typename>
struct is_convertible_to_function_pointer {
    static constexpr bool value = false;
};

template <typename Ret, typename... Args>
struct is_convertible_to_function_pointer<Ret (*)(Args...)> {
    static constexpr bool value = true;
};

template <typename T, typename = decltype(&T::operator())>
struct _is_class_convertible_to_function_pointer : std::false_type {};

template <typename Ret, typename T, typename... Args>
struct _is_class_convertible_to_function_pointer<T, Ret (T::*)(Args...)> {
    using free_function_t       = Ret (*)(Args...);
    static constexpr bool value = std::convertible_to<T, free_function_t>;
};
template <typename Ret, typename T, typename... Args>
struct _is_class_convertible_to_function_pointer<T, Ret (T::*)(Args...) const> {
    using free_function_t       = Ret (*)(Args...);
    static constexpr bool value = std::convertible_to<T, free_function_t>;
};
template <typename Ret, typename T, typename... Args>
struct _is_class_convertible_to_function_pointer<
    T, Ret (T::*)(Args...) noexcept> {
    using free_function_t       = Ret (*)(Args...);
    static constexpr bool value = std::convertible_to<T, free_function_t>;
};
template <typename Ret, typename T, typename... Args>
struct _is_class_convertible_to_function_pointer<
    T, Ret (T::*)(Args...) const noexcept> {
    using free_function_t       = Ret (*)(Args...);
    static constexpr bool value = std::convertible_to<T, free_function_t>;
};

template <typename Ty>
requires std::is_class_v<Ty> && requires() { &Ty::operator(); }
struct is_convertible_to_function_pointer<Ty> {
    static constexpr bool value =
        _is_class_convertible_to_function_pointer<Ty>::value;
};

template <typename Ty>
constexpr bool is_convertible_to_function_pointer_v =
    is_convertible_to_function_pointer<Ty>::value;

} // namespace neutron
