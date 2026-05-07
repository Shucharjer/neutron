#pragma once

namespace neutron {

template <typename>
struct is_function_pointer {
    static constexpr bool value = false;
};

template <typename Ret, typename... Args>
struct is_function_pointer<Ret (*)(Args...)> {
    static constexpr bool value = true;
};

template <typename Ty>
constexpr bool is_function_pointer_v = is_function_pointer<Ty>::value;

} // namespace neutron
