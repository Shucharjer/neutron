/**
 * @brief This file could help us build pipeline system.
 * The operator| needs to provide by the nearest implementation.
 *
 * @date 2025-11-21
 */

/// @note Here is an example how you use this header file to build your own
/// closure system quickly:

// template <typename Derived>
// struct custom_adaptor_closure : adaptor_closure<Derived> {};

// template <typename Ty>
// concept _custom_adaptor_closure_object = _adaptor_closure<Ty,
// adaptor_closure>;

// template <typename First, typename Second>
// struct _custom_closure_compose :
//     public _closure_compose<First, Second, custom_adaptor_closure> {
//     using _compose_base =
//         _closure_compose<First, Second, custom_adaptor_closure>;
//     using _compose_base::_compose_base;
//     using _compose_base::operator();
// };

// template <typename C1, typename C2>
// _custom_closure_compose(C1&&, C2&&) -> _custom_closure_compose<
//     std::remove_cvref_t<C1>, std::remove_cvref_t<C2>>;

// template <typename Ty>
// concept _your_custom_concept = false;

// template <typename Closure, typename Arg>
// concept _your_custom_concept_adaptor_closure_for =
//     _your_custom_concept<Arg> && _custom_adaptor_closure_object<Closure> &&
//     _your_custom_concept<std::invoke_result_t<Closure, Arg>>;

// template <
//     _your_custom_concept Arg,
//     _your_custom_concept_adaptor_closure_for<Arg> Closure>
// constexpr decltype(auto) operator|(Arg&& arg, Closure&& closure) {
//     return std::forward<Closure>(closure)(std::forward<Arg>(arg));
// }

// template <
//     _custom_adaptor_closure_object Closure1,
//     _custom_adaptor_closure_object Closure2>
// constexpr auto operator|(Closure1&& closure1, Closure2&& closure2)
//     -> _custom_closure_compose<
//         std::remove_cvref_t<Closure1>, std::remove_cvref_t<Closure2>> {
//     return _custom_closure_compose(
//         std::forward<Closure1>(closure1), std::forward<Closure2>(closure2));
// }

#pragma once
#include <concepts>
#include <type_traits>
#include <utility>
#include "neutron/pair.hpp"
#include "./macros.hpp"

namespace neutron {

template <typename Derived>
struct adaptor_closure {
    constexpr adaptor_closure() noexcept = default;
};

template <typename Ty, template <typename> typename AdaptorClosure>
concept _adaptor_closure =
    std::derived_from<
        std::remove_cvref_t<Ty>, AdaptorClosure<std::remove_cvref_t<Ty>>> &&
    std::move_constructible<std::remove_cvref_t<Ty>> &&
    std::constructible_from<std::remove_cvref_t<Ty>, Ty>;

template <
    typename Ty, template <typename> typename AdaptorClosure, typename Arg>
concept _adaptor_closure_for =
    _adaptor_closure<Ty, AdaptorClosure> && std::is_invocable_v<Ty, Arg>;

template <
    template <typename, typename> typename Derived,
    template <typename> typename AdaptorClosure, typename First,
    typename Second>
class _closure_compose : public AdaptorClosure<Derived<First, Second>> {
public:
    template <typename Clousre1, typename Closure2>
    constexpr _closure_compose(Clousre1&& closure1, Closure2&& closure2)
        : pair_(
              std::forward<Clousre1>(closure1),
              std::forward<Closure2>(closure2)) {}

    /**
     * @brief Get out of the pipeline.
     *
     * Usually, the tparam will be a range, but not only.
     * It depends on the closure.
     */
    template <typename Arg>
    requires std::is_invocable_v<First, Arg> &&
             std::is_invocable_v<Second, std::invoke_result_t<First, Arg>>
    NODISCARD constexpr auto operator()(Arg&& arg) const& noexcept(
        noexcept((pair_.second())(pair_.first()(std::forward<Arg>(arg))))) {
        return (pair_.second())(pair_.first()(std::forward<Arg>(arg)));
    }

    /**
     * @brief Get out of the pipeline.
     *
     * Usually, the tparam will be a range, but not only.
     * It depends on the closure.
     */
    template <typename Arg>
    requires std::is_invocable_v<First, Arg> &&
             std::is_invocable_v<Second, std::invoke_result_t<First, Arg>>
    NODISCARD constexpr auto
        operator()(Arg&& arg) && noexcept(noexcept(std::move(pair_.second())(
            std::move(pair_.first())(std::forward<Arg>(arg))))) {
        return std::move(pair_.second())(
            std::move(pair_.first())(std::forward<Arg>(arg)));
    }

private:
    compressed_pair<First, Second> pair_;
};

} // namespace neutron
