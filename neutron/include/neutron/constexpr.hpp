#pragma once
#include "neutron/type_list.hpp"

namespace neutron {

namespace internal {

template <auto Expr, typename ExprTy = decltype(Expr)>
struct _expression_traits;

template <auto Expr, typename Ret, typename... Args>
struct _expression_traits<Expr, Ret (*)(Args...)> {
    constexpr static std::size_t args_count = sizeof...(Args);
    using args_type                         = type_list<Args...>;
    constexpr static bool is_constexpr =
        requires { typename std::bool_constant<(Expr(Args{}...), false)>; };
};

template <auto Expr, typename Ret, typename Clazz, typename... Args>
struct _expression_traits<Expr, Ret (Clazz::*)(Args...)> {
    constexpr static std::size_t args_count = sizeof...(Args);
    using args_type                         = type_list<Args...>;
    constexpr static bool is_constexpr =
        requires { typename std::bool_constant<(Clazz{}.*Expr(Args()...), false)>; };
};

template <auto Expr, typename Ret, typename Clazz, typename... Args>
struct _expression_traits<Expr, Ret (Clazz::*)(Args...) const> {
    constexpr static std::size_t args_count = sizeof...(Args);
    using args_type                         = type_list<Args...>;
    constexpr static bool is_constexpr =
        requires { typename std::bool_constant<(Clazz{}.*Expr(Args{}...), false)>; };
};

template <auto Expr, typename ExprTy>
struct _expression_traits {
    static_assert(std::is_class_v<ExprTy>);

    template <typename>
    struct helper;

    template <typename Ret, typename Clazz, typename... Args>
    struct helper<Ret (Clazz::*)(Args...) const> {
        constexpr static Ret invoke() { return Expr(Args{}...); }

        constexpr static std::size_t args_count = sizeof...(Args);
        using args_type                         = type_list<Args...>;
        constexpr static bool is_constexpr =
            requires { typename std::bool_constant<(invoke(), false)>; };
    };

    using helper_t = helper<decltype(&ExprTy::operator())>;

    constexpr static std::size_t args_count = helper_t::args_count;
    using args_type                         = helper_t::args_type;
    constexpr static bool is_constexpr      = helper_t::is_constexpr;
};

} // namespace internal

/*! @endcond */

/**
 * @brief Check if an expression is a compile-time constant.
 * Generally, you could judge whether a function is a compile-time constant by checking if it is a
 * `constexpr` function. But this function is more powerful, it could be used to checking when you
 * are trying writing a template.
 * Inspired by `is_constexpr` in microsoft porxy.
 */
template <auto Expr>
consteval bool is_constexpr() {
    return internal::_expression_traits<Expr>::is_constexpr;
}

}