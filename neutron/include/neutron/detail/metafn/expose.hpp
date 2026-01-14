// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <concepts>
#include "neutron/detail/metafn/cat.hpp"
#include "neutron/detail/metafn/convert.hpp"

namespace neutron {

template <template <typename...> typename Tmp, typename TypeList>
struct type_list_expose;
template <
    template <typename...> typename Tmp,
    template <typename...> typename Template>
struct type_list_expose<Tmp, Template<>> {
    using type = Template<>;
};
template <
    template <typename...> typename Tmp,
    template <typename...> typename Template, typename... Tys>
struct type_list_expose<Tmp, Template<Tys...>> {
    template <typename Ty>
    struct expose {
        using type = Template<Ty>;
    };
    template <typename... Args>
    struct expose<Tmp<Args...>> {
        using type = Template<Args...>;
    };

    using type = type_list_cat_t<typename expose<Tys>::type...>;
};
template <template <typename...> typename Tmp, typename TypeList>
using type_list_expose_t = typename type_list_expose<Tmp, TypeList>::type;

/**
 * @tparam To The target type.
 * @tparam From The *reference* of the target type. It is ignored here.
 * @note Matter the identifier of a type by using class template like `same_cv`.
 * @note The reference do not means something like `&`.
 */
template <typename To, typename Reference>
using ignore_cvref = std::remove_cvref<To>;

/** @brief Recursively flattens nested instantiations of a given template within
 *         a type list.
 *
 * This metafunction traverses a type list (e.g., `Template<Tys...>`) and:
 * - If an element matches the target template `Tmp<Args...>`, it recursively
 *   expands `Args...`.
 * - Otherwise, the element is preserved as-is (wrapped in the outer
 *   `Template`).
 * - CV-qualified or reference types are handled via the optional `Qualifier`
 *   policy.
 *
 * @tparam Tmp        The template whose nested instantiations should be
 *                    expanded (e.g., `std::tuple`).
 * @tparam TypeList   A type container of the form `Template<Tys...>` (e.g.,
 *                    `std::tuple<int, float>`).
 * @tparam Qualifier  A binary template used to reapply cv/ref qualifiers when
 *                    needed. Defaults to `ignore_cvref`, which discards
 *                    qualifiers.
 */
template <
    template <typename...> typename Tmp, typename TypeList,
    template <typename, typename> typename Qualifier = ignore_cvref>
struct type_list_recurse_expose;
/** @brief Specialization for an empty type list.
 *
 * When the input `TypeList` contains no types (i.e., `Template<>`), the result
 * is simply the same empty list.
 */
template <
    template <typename...> typename Tmp,
    template <typename...> typename Template,
    template <typename, typename> typename Qualifier>
struct type_list_recurse_expose<Tmp, Template<>, Qualifier> {
    using type = Template<>;
};

/** @brief Primary specialization for non-empty type lists.
 *
 * Processes each type in `Template<Tys...>`:
 * - If a type is an instantiation of `Tmp<Args...>`, its arguments are
 *   recursively exposed.
 * - Otherwise, the type is wrapped in `Template<Ty>`.
 * - CV/ref qualifiers are stripped before processing and optionally reapplied
 *   using `Qualifier`.
 *
 * @tparam Tmp        Target template to expand (e.g., `std::variant`).
 * @tparam Template   The outer type list template (e.g., `std::tuple`).
 * @tparam Tys        The types contained in the input list.
 * @tparam Qualifier  Policy for handling qualifiers on non-primitive types.
 */
template <
    template <typename...> typename Tmp,
    template <typename...> typename Template, typename... Tys,
    template <typename, typename> typename Qualifier>
struct type_list_recurse_expose<Tmp, Template<Tys...>, Qualifier> {
    template <typename Ty>
    struct try_expose;
    template <typename Ty>
    struct expose {
        using type = Template<Ty>;
    };
    template <typename... Args>
    struct expose<Tmp<Args...>> {
        using type = type_list_cat_t<typename try_expose<Args>::type...>;
    };
    template <typename Ty>
    requires std::same_as<std::remove_cvref_t<Ty>, Ty>
    struct try_expose<Ty> {
        using type = typename expose<Ty>::type;
    };
    template <typename Ty>
    requires(!std::same_as<std::remove_cvref_t<Ty>, Ty>)
    struct try_expose<Ty> {
        template <typename T>
        using predicate_type = Qualifier<T, Ty>;
        using type           = type_list_convert_t<
                      predicate_type, typename expose<std::remove_cvref_t<Ty>>::type>;
    };
    using type = type_list_cat_t<typename try_expose<Tys>::type...>;
};
/** @brief Alias template for convenient access to the result type.
 *
 * Provides a shorthand for `typename type_list_recurse_expose<...>::type`.
 *
 * @tparam Tmp        The template to recursively expand.
 * @tparam TypeList   Input type list (e.g., `std::tuple<A, B, std::tuple<C>>`).
 * @tparam Qualifier  Qualifier handling policy (default: `ignore_cvref`).
 */
template <
    template <typename...> typename Tmp, typename TypeList,
    template <typename, typename> typename Qualifier = ignore_cvref>
using type_list_recurse_expose_t =
    typename type_list_recurse_expose<Tmp, TypeList, Qualifier>::type;

} // namespace neutron
