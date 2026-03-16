// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <type_traits>

namespace neutron {

/// @brief Metafunction that substitutes all occurrences of a type `Old` with
/// `New`
///        in a type list represented as an instantiation of a variadic
///        template.
///
/// This metafunction works on any template that takes a parameter pack of
/// types, such as `std::tuple`, `std::variant`, or user-defined templates like
/// `template<typename...> struct my_list;`.
///
/// For example:
/// @code
/// using Input = std::tuple<int, float, int>;
/// using Output = type_list_substitute_t<Input, int, long>;
/// // Output is std::tuple<long, float, long>
/// @endcode
///
/// @tparam Template  An instantiated variadic template type (e.g.,
/// `std::tuple<int, char>`)
/// @tparam Old       The type to be replaced
/// @tparam New       The type to replace `Old` with
///
/// @note Only direct occurrences of `Old` are replaced; nested types (e.g.,
/// inside
///       `std::vector<Old>`) are not affected.
template <typename Template, typename Old, typename New>
struct type_list_substitute;
template <
    template <typename...> typename Template, typename... Tys, typename Old,
    typename New>
struct type_list_substitute<Template<Tys...>, Old, New> {
    template <typename Curr, typename... Remains>
    struct substitute;
    template <typename... Curr>
    struct substitute<Template<Curr...>> {
        using type = Template<Curr...>;
    };
    template <typename... Curr, typename Ty>
    struct substitute<Template<Curr...>, Ty> {
        using type = std::conditional_t<
            std::is_same_v<Ty, Old>, Template<Curr..., New>,
            Template<Curr..., Ty>>;
    };
    template <typename... Curr, typename Ty, typename... Others>
    struct substitute<Template<Curr...>, Ty, Others...> {
        using type = std::conditional_t<
            std::is_same_v<Ty, Old>, Template<Curr..., New, Others...>,
            typename substitute<Template<Curr..., Ty>, Others...>::type>;
    };
    using type = typename substitute<Template<>, Tys...>::type;
};
template <typename Template, typename Old, typename New>
using type_list_substitute_t =
    typename type_list_substitute<Template, Old, New>::type;

} // namespace neutron
