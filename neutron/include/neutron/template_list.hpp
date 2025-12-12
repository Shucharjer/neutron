#pragma once
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include "neutron/shared_tuple.hpp"
#include "neutron/type_traits.hpp"
#include "../src/neutron/internal/get.hpp"

namespace neutron {

template <auto... Vals>
struct value_list {};

template <typename>
struct is_value_list : std::false_type {};
template <template <auto...> typename Template, auto... Vals>
struct is_value_list<Template<Vals...>> : std::true_type {};
template <typename Ty>
constexpr auto is_value_list_v = is_value_list<Ty>::value;

template <template <auto...> typename Template, typename>
struct is_specific_value_list : std::false_type {};
template <template <auto...> typename Template, auto... Vals>
struct is_specific_value_list<Template, Template<Vals...>> : std::true_type {};
template <template <auto...> typename Template, typename Ty>
constexpr bool is_specific_value_list_v =
    is_specific_value_list<Template, Ty>::value;

template <typename Ty>
struct value_list_size;
template <auto... Vals>
struct value_list_size<value_list<Vals...>> {
    constexpr static std::size_t value = sizeof...(Vals);
};
template <typename Ty>
constexpr size_t value_list_size_v = value_list_size<Ty>::value;

template <size_t Index, typename Ty>
struct value_list_element;
template <auto Val, auto... Others>
struct value_list_element<0, value_list<Val, Others...>> {
    constexpr static auto value = Val;
};
template <size_t Index, auto Val, auto... Others>
struct value_list_element<Index, value_list<Val, Others...>> {
    constexpr static auto value =
        value_list_element<Index - 1, value_list<Others...>>::value;
};
template <size_t Index, typename Ty>
constexpr inline auto value_list_element_v =
    value_list_element<Index, Ty>::value;

template <typename...>
struct value_list_cat;
template <template <auto...> typename Template>
struct value_list_cat<Template<>> {
    using type = Template<>;
};
template <template <auto...> typename Template, auto... Vals>
struct value_list_cat<Template<Vals...>> {
    using type = Template<Vals...>;
};
template <template <auto...> typename Template, auto... Vals1, auto... Vals2>
struct value_list_cat<Template<Vals1...>, Template<Vals2...>> {
    using type = Template<Vals1..., Vals2...>;
};
template <typename Vl1, typename Vl2, typename... Vls>
struct value_list_cat<Vl1, Vl2, Vls...> {
    using type = typename value_list_cat<
        typename value_list_cat<Vl1, Vl2>::type, Vls...>::type;
};
template <typename... Vls>
using value_list_cat_t = typename value_list_cat<Vls...>::type;

template <auto Lhs, auto Rhs>
struct is_same_value {
    template <auto Val>
    struct value_wrapper {};
    constexpr static bool value =
        std::is_same_v<value_wrapper<Lhs>, value_wrapper<Rhs>>;
};
template <auto Lhs, auto Rhs>
constexpr auto is_same_value_v = is_same_value<Lhs, Rhs>::value;

template <auto Val, typename>
struct value_list_has : std::false_type {};
template <auto Val, template <auto...> typename Template, auto... Vals>
struct value_list_has<Val, Template<Vals...>> {
    constexpr static bool value = (is_same_value_v<Val, Vals> || ...);
};
template <auto Val, typename Vl>
constexpr bool value_list_has_v = value_list_has<Val, Vl>::value;

template <typename>
struct value_list_first;
template <template <auto...> typename Template, auto Val, auto... Others>
struct value_list_first<Template<Val, Others...>> {
    constexpr static auto value = Val;
};
template <typename ValList>
constexpr auto value_list_first_v = value_list_first<ValList>::value;

template <template <auto...> typename List, typename>
struct append_value_list;
template <
    template <auto...> typename List, template <typename...> typename Template,
    typename... Tys>
struct append_value_list<List, Template<Tys...>> {
    using type = Template<Tys..., List<>>;
};
template <template <auto...> typename List, typename Target>
using append_value_list_t = typename append_value_list<List, Target>::type;

template <template <auto> typename Predicate, typename ValList>
struct value_list_filt;
template <
    template <auto> typename Predicate, template <auto...> typename Template,
    auto... Tys>
struct value_list_filt<Predicate, Template<Tys...>> {
    template <typename Curr, auto... Remains>
    struct filt;
    template <auto... Curr>
    struct filt<Template<Curr...>> {
        using type = Template<Curr...>;
    };
    template <auto... Curr, auto Val>
    struct filt<Template<Curr...>, Val> {
        using type = std::conditional_t<
            Predicate<Val>::value, Template<Curr..., Val>, Template<Curr...>>;
    };
    template <auto... Curr, auto Val, auto... Others>
    struct filt<Template<Curr...>, Val, Others...> {
        using type = std::conditional_t<
            Predicate<Val>::value,
            typename filt<Template<Curr..., Val>, Others...>::type,
            typename filt<Template<Curr...>, Others...>::type>;
    };
    using type = typename filt<Template<>, Tys...>::type;
};
template <template <auto> typename Predicate, typename ValList>
using value_list_filt_t = typename value_list_filt<Predicate, ValList>::type;

// template <template<auto>typename Predicate, typename ValList, typename
// TyIfEmpty> struct value_list_filt_nvoid{
//     template <
// };

/**
 * @brief Make a tuple from a value_list.
 */
template <typename Ty>
requires is_value_list_v<Ty>
constexpr auto make_tuple() noexcept {
    return []<size_t... Is>(std::index_sequence<Is...>) {
        return std::make_tuple(value_list_element_v<Is, Ty>...);
    }(std::make_index_sequence<value_list_size_v<Ty>>());
}
/**
 * @brief Make a tuple from a value_list.
 */
template <typename Ty>
requires is_value_list_v<Ty>
constexpr auto make_tuple(Ty) noexcept {
    return []<size_t... Is>(std::index_sequence<Is...>) {
        return std::make_tuple(value_list_element_v<Is, Ty>...);
    }(std::make_index_sequence<value_list_size_v<Ty>>());
}

/**
 * @class type_list
 * @brief A class stores type lighter than tuple.
 */
template <typename...>
struct type_list {};

template <typename>
struct type_list_size;
template <template <typename...> typename Template, typename... Tys>
struct type_list_size<Template<Tys...>> {
    constexpr static size_t value = sizeof...(Tys);
};
template <typename Ty>
constexpr static auto type_list_size_v = type_list_size<Ty>::value;

template <std::size_t Index, typename TypeList>
struct type_list_element;

template <
    template <typename...> typename Template, typename Ty, typename... Args>
struct type_list_element<0, Template<Ty, Args...>> {
    using type = Ty;
};
template <
    std::size_t Index, template <typename...> typename Template, typename Ty,
    typename... Args>
struct type_list_element<Index, Template<Ty, Args...>> {
    using type = type_list_element<Index - 1, Template<Args...>>::type;
};
template <std::size_t Index, typename TypeList>
using type_list_element_t = typename type_list_element<Index, TypeList>::type;

template <typename... Tls>
struct type_list_cat;
template <template <typename...> typename Template, typename... Tys>
struct type_list_cat<Template<Tys...>> {
    using type = Template<Tys...>;
};
template <
    template <typename...> typename Template, typename... Tys1,
    typename... Tys2>
struct type_list_cat<Template<Tys1...>, Template<Tys2...>> {
    using type = Template<Tys1..., Tys2...>;
};
template <typename Tl1, typename Tl2, typename... Tls>
struct type_list_cat<Tl1, Tl2, Tls...> {
    using type = typename type_list_cat<
        typename type_list_cat<Tl1, Tl2>::type, Tls...>::type;
};
template <typename... Tls>
using type_list_cat_t = typename type_list_cat<Tls...>::type;

template <typename>
struct value_list_type_list_cat;
template <template <auto...> typename Template, auto... TypeList>
struct value_list_type_list_cat<Template<TypeList...>> {
    using type = type_list_cat_t<std::remove_cvref_t<decltype(TypeList)>...>;
};
template <typename TypeValueList>
using value_list_type_list_cat_t =
    typename value_list_type_list_cat<TypeValueList>::type;

template <auto Val>
struct to_type {
    using type = decltype(Val);
};

template <auto Val>
struct to_raw_type {
    using type = std::remove_cvref_t<decltype(Val)>;
};

template <
    template <auto> typename Predicate, typename ValList,
    template <typename...> typename Tmp = type_list>
struct value_list_to;
template <
    template <auto> typename Predicate, template <auto...> typename ValList,
    template <typename...> typename Tmp>
struct value_list_to<Predicate, ValList<>, Tmp> {
    using type = Tmp<>;
};
template <
    template <auto> typename Predicate, template <auto...> typename Template,
    auto... Vals, template <typename...> typename Tmp>
struct value_list_to<Predicate, Template<Vals...>, Tmp> {
    using type = type_list_cat_t<Tmp<typename Predicate<Vals>::type>...>;
};
template <
    template <auto> typename Predicate, typename ValList,
    template <typename...> typename Tmp = type_list>
using value_list_to_t = typename value_list_to<Predicate, ValList, Tmp>::type;

template <typename, typename>
struct type_list_has : std::false_type {};
template <
    typename Ty, template <typename...> typename Template, typename... Types>
struct type_list_has<Ty, Template<Types...>> {
    constexpr static bool value = (std::is_same_v<Ty, Types> || ...);
};
template <typename Ty, typename Tuple>
constexpr bool type_list_has_v = type_list_has<Ty, Tuple>::value;

template <typename>
struct empty_type_list;
template <template <typename...> typename Template, typename... Tys>
struct empty_type_list<Template<Tys...>> {
    using type = Template<>;
};
template <typename TypeList>
using empty_type_list_t = typename empty_type_list<TypeList>::type;

template <typename TypeList, typename = empty_type_list_t<TypeList>>
struct unique_type_list;
template <template <typename...> typename Template, typename Current>
struct unique_type_list<Template<>, Current> {
    using type = Current;
};
template <
    template <typename...> typename Template, typename Ty, typename... Others,
    typename Prev>
struct unique_type_list<Template<Ty, Others...>, Prev> {
    using current_list =
        std::conditional_t<type_list_has_v<Ty, Prev>, Template<>, Template<Ty>>;
    using type = unique_type_list<
        Template<Others...>, type_list_cat_t<Prev, current_list>>::type;
};
template <typename Tl>
using unique_type_list_t = typename unique_type_list<Tl>::type;

template <typename Ty>
struct is_type_list : std::false_type {};
template <template <typename...> typename Template, typename... Tys>
struct is_type_list<Template<Tys...>> : std::true_type {};
template <typename Ty>
constexpr auto is_type_list_v = is_type_list<Ty>::value;

template <template <typename...> typename Template, typename Ty>
struct is_specific_type_list : std::false_type {};
template <template <typename...> typename Template, typename... Tys>
struct is_specific_type_list<Template, Template<Tys...>> : std::true_type {};
template <template <typename...> typename Template, typename Ty>
constexpr auto is_specific_type_list_v =
    is_specific_type_list<Template, Ty>::value;

template <typename Ty>
struct is_empty_template : std::false_type {};
template <template <typename...> typename Template>
struct is_empty_template<Template<>> : std::true_type {};
template <template <auto...> typename Template>
struct is_empty_template<Template<>> : std::true_type {};
template <typename Ty>
constexpr auto is_empty_template_v = is_empty_template<Ty>::value;

template <typename Fallback, typename Ty>
struct type_list_not_empty;
template <typename Fallback, typename Ty>
requires std::negation_v<is_empty_template<Ty>>
struct type_list_not_empty<Fallback, Ty> {
    using type = Ty;
};
template <typename Fallback, typename Ty>
requires is_empty_template_v<Ty>
struct type_list_not_empty<Fallback, Ty> {
    using type = Fallback;
};
template <typename Fallback, typename Ty>
using type_list_not_empty_t = typename type_list_not_empty<Fallback, Ty>::type;

template <template <typename...> typename List, typename>
struct append_type_list;
template <
    template <typename...> typename List,
    template <typename...> typename Template, typename... Tys>
struct append_type_list<List, Template<Tys...>> {
    using type = Template<Tys..., List<>>;
};
template <template <typename...> typename List, typename Target>
using append_type_list_t = typename append_type_list<List, Target>::type;

/**
 * @class type_list_filt
 * @brief Filter the types in the list to only those for which the predicate is
 * true.
 */
template <template <typename> typename Predicate, typename Template>
struct type_list_filt;
template <
    template <typename> typename Predicate,
    template <typename...> typename Template, typename... Tys>
struct type_list_filt<Predicate, Template<Tys...>> {
    template <typename Curr, typename... Remains>
    struct filt;
    template <typename... Curr>
    struct filt<Template<Curr...>> {
        using type = Template<Curr...>;
    };
    template <typename... Curr, typename Ty>
    struct filt<Template<Curr...>, Ty> {
        using type = std::conditional_t<
            Predicate<Ty>::value, Template<Curr..., Ty>, Template<Curr...>>;
    };
    template <typename... Curr, typename Ty, typename... Others>
    struct filt<Template<Curr...>, Ty, Others...> {
        using type = std::conditional_t<
            Predicate<Ty>::value,
            typename filt<Template<Curr..., Ty>, Others...>::type,
            typename filt<Template<Curr...>, Others...>::type>;
    };
    using type = typename filt<Template<>, Tys...>::type;
};
template <template <typename> typename Predicate, typename Template>
using type_list_filt_t = typename type_list_filt<Predicate, Template>::type;

template <typename, typename>
struct type_list_contains;
template <
    typename Ty, template <typename...> typename Template, typename... Tys>
struct type_list_contains<Ty, Template<Tys...>> :
    std::bool_constant<(std::is_same_v<Ty, Tys> || ...)> {};
template <typename Ty, typename TypeList>
constexpr auto type_list_contains_v = type_list_contains<Ty, TypeList>::value;

template <typename>
struct type_list_first;
template <
    template <typename...> typename Template, typename Ty, typename... Others>
struct type_list_first<Template<Ty, Others...>> {
    using type = Ty;
};
template <typename Tl>
using type_list_first_t = typename type_list_first<Tl>::type;

template <typename>
struct type_list_last;
template <template <typename...> typename Template, typename Ty>
struct type_list_last<Template<Ty>> {
    using type = Ty;
};
template <
    template <typename...> typename Template, typename Ty, typename... Others>
struct type_list_last<Template<Ty, Others...>> {
    using type = typename type_list_last<Others...>::type;
};
template <typename Tl>
using type_list_last_t = typename type_list_last<Tl>::type;

template <
    template <typename> typename Predicate, typename TypeList,
    typename TyIfEmpty>
struct type_list_filt_nvoid {
    using type = std::remove_pointer_t<decltype([] {
        using filted_type = type_list_filt_t<Predicate, TypeList>;
        if constexpr (is_empty_template_v<filted_type>) {
            return static_cast<TyIfEmpty*>(nullptr);
        } else {
            return static_cast<filted_type*>(nullptr);
        }
    }())>;
};
template <
    template <typename> typename Predicate, typename TypeList,
    typename TyIfEmpty>
using type_list_filt_nvoid_t =
    type_list_filt_nvoid<Predicate, TypeList, TyIfEmpty>::type;

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

template <template <typename...> typename Tmp, typename TypeList>
struct type_list_export;
template <
    template <typename...> typename Tmp,
    template <typename...> typename Template>
struct type_list_export<Tmp, Template<>> {
    using type = Tmp<>;
};
template <
    template <typename...> typename Tmp,
    template <typename...> typename Template, typename... Tys>
struct type_list_export<Tmp, Template<Tys...>> {
    template <typename Ty>
    struct expose {
        using type = Tmp<>;
    };
    template <typename... Args>
    struct expose<Tmp<Args...>> {
        using type = Tmp<Args...>;
    };
    using type = type_list_cat_t<typename expose<Tys>::type...>;
};
template <template <typename...> typename Tmp, typename TypeList>
using type_list_export_t = typename type_list_export<Tmp, TypeList>::type;

template <template <auto...> typename Tmp, typename ValueTypeList>
struct value_list_export;
template <
    template <auto...> typename Tmp, template <typename...> typename Template>
struct value_list_export<Tmp, Template<>> {
    using type = Tmp<>;
};
template <
    template <auto...> typename Tmp, template <typename...> typename Template,
    typename... Tys>
struct value_list_export<Tmp, Template<Tys...>> {
    template <typename Ty>
    struct expose {
        using type = Tmp<>;
    };
    template <auto... Args>
    struct expose<Tmp<Args...>> {
        using type = Tmp<Args...>;
    };
    using type = value_list_cat_t<typename expose<Tys>::type...>;
};
template <template <auto...> typename Tmp, typename ValueTypeList>
using value_list_export_t =
    typename value_list_export<Tmp, ValueTypeList>::type;

template <template <typename...> typename Template, typename TypeList>
struct type_list_filt_type_list {
    template <typename Ty>
    using _is_template = is_specific_type_list<Template, Ty>;
    using type         = type_list_export_t<
                Template, type_list_filt_nvoid_t<_is_template, TypeList, Template<>>>;
};
template <template <typename...> typename Template, typename TypeList>
using type_list_filt_type_list_t =
    typename type_list_filt_type_list<Template, TypeList>::type;

template <template <auto...> typename Template, typename TypeList>
struct type_list_filt_value_list {
    template <typename Ty>
    using _is_template = is_specific_value_list<Template, Ty>;
    using type = type_list_filt_nvoid_t<_is_template, TypeList, Template<>>;
};
template <template <auto...> typename Template, typename TypeList>
using type_list_filt_value_list_t =
    typename type_list_filt_value_list<Template, TypeList>::type;

template <typename TypeList1, typename TypeList2>
struct type_list_all_differs_from;
template <
    template <typename...> typename Template1,
    template <typename...> typename Template2, typename... Tys1,
    typename... Tys2>
struct type_list_all_differs_from<Template1<Tys1...>, Template2<Tys2...>> {
    constexpr static bool value =
        !(type_list_contains_v<Tys1, Template2<Tys2...>> || ...);
};
template <typename TypeList1, typename TypeList2>
constexpr auto type_list_all_differs_from_v =
    type_list_all_differs_from<TypeList1, TypeList2>::value;

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

template <typename Ty>
struct always_true : std::true_type {};

template <
    template <typename> typename Predicate, typename TypeList,
    template <typename> typename Continue  = always_true,
    template <typename> typename Qualifier = std::type_identity_t>
struct type_list_requires_recurse {
    template <typename Ty>
    struct requires_recurse;
    template <typename Ty>
    struct try_requires_recurse {
        constexpr static bool value = requires_recurse<Qualifier<Ty>>::value;
    };
    template <typename Ty>
    requires std::same_as<Ty, std::remove_cvref_t<Ty>>
    struct try_requires_recurse<Ty> {
        constexpr static bool value = requires_recurse<Ty>::value;
    };
    template <typename Ty>
    struct requires_recurse {
        constexpr static bool value = Predicate<Ty>::value;
    };
    template <template <typename...> typename Template, typename... Tys>
    requires Continue<Template<Tys...>>::value &&
             (!Predicate<Template<Tys...>>::value)
    struct requires_recurse<Template<Tys...>> {
        constexpr static bool value = (try_requires_recurse<Tys>::value && ...);
    };
    constexpr static bool value = try_requires_recurse<TypeList>::value;
};
template <
    template <typename> typename Predicate, typename TypeList,
    template <typename> typename Continue = always_true,
    template <typename> typename Qualifer = std::type_identity_t>
constexpr auto type_list_requires_recurse_v =
    type_list_requires_recurse<Predicate, TypeList, Continue>::value;

template <template <auto> typename, typename>
struct type_list_from_value;
template <
    template <auto> typename Predicate, template <auto...> typename Template,
    auto... Vals>
struct type_list_from_value<Predicate, Template<Vals...>> {
    using type = neutron::type_list<typename Predicate<Vals>::type...>;
};
template <template <auto> typename Predicate, typename ValList>
using type_list_from_value_t =
    typename type_list_from_value<Predicate, ValList>::type;

template <template <auto, typename...> typename Tmp, auto Value, typename>
struct type_list_with_value;
template <
    template <auto, typename...> typename Tmp, auto Value,
    template <typename...> typename Template, typename... Tys>
struct type_list_with_value<Tmp, Value, Template<Tys...>> {
    using type = Tmp<Value, Tys...>;
};
template <
    template <auto, typename...> typename Tmp, auto Value, typename TypeList>
using type_list_with_value_t =
    typename type_list_with_value<Tmp, Value, TypeList>::type;

template <
    template <typename...> typename Template, typename Ty, typename Target>
struct insert_type_list_inplace {
    template <typename T>
    using _is_the_type_list = is_specific_type_list<Template, T>;
    using type              = std::remove_pointer_t<decltype([]() {
        using filted_type = type_list_filt_t<_is_the_type_list, Target>;
        if constexpr (is_empty_template<filted_type>::value) {
            using fixed_type =
                typename append_type_list<Template, Target>::type;
            using filted_type = type_list_filt_t<_is_the_type_list, fixed_type>;
            using current_type = type_list_first_t<filted_type>;
            using result       = type_list_substitute_t<
                                   fixed_type, current_type,
                                   type_list_cat_t<current_type, Template<Ty>>>;
            return static_cast<result*>(nullptr);
        } else {
            using current_type = type_list_first_t<filted_type>;
            using result       = type_list_substitute_t<
                                   Target, current_type,
                                   type_list_cat_t<current_type, Template<Ty>>>;
            return static_cast<result*>(nullptr);
        }
    }())>;
};
template <
    template <typename...> typename Template, typename Ty, typename Target>
using insert_type_list_inplace_t =
    typename insert_type_list_inplace<Template, Ty, Target>::type;

template <typename Ty, typename List>
struct type_list_remove;
template <
    typename Ty, template <typename...> typename Template, typename... Tys>
struct type_list_remove<Ty, Template<Tys...>> {
    template <typename T>
    using predicate_type = std::negation<std::is_same<Ty, T>>;
    using type           = type_list_filt_t<predicate_type, Template<Tys...>>;
};
template <typename Ty, typename List>
using type_list_remove_t = typename type_list_remove<Ty, List>::type;

template <template <typename> typename Predicate, typename TypeList>
struct type_list_erase_if {
    template <typename Ty>
    using _predicate_type = std::negation<Predicate<Ty>>;
    using type            = type_list_filt_t<_predicate_type, TypeList>;
};
template <template <typename> typename Predicate, typename TypeList>
using type_list_erase_if_t =
    typename type_list_erase_if<Predicate, TypeList>::type;

template <typename List, typename Ty>
struct in_type_list;
template <
    template <typename...> typename Template, typename... Tys, typename Ty>
struct in_type_list<Template<Tys...>, Ty> {
    constexpr static bool value = (std::is_same_v<Tys, Ty> || ...);
};
template <typename List, typename Ty>
constexpr auto in_type_list_v = in_type_list<List, Ty>::value;

template <typename List, typename Target>
struct type_list_remove_in;
template <
    template <typename...> typename ListTemplate, typename... ListTys,
    typename Target>
struct type_list_remove_in<ListTemplate<ListTys...>, Target> {
    template <typename T>
    using predicate_type =
        std::negation<in_type_list<ListTemplate<ListTys...>, T>>;
    using type = type_list_filt_t<predicate_type, Target>;
};
template <typename List, typename Target>
using type_list_remove_in_t = typename type_list_remove_in<List, Target>::type;

template <typename TypeList>
struct type_list_pop_first;
template <
    template <typename...> typename Template, typename Ty, typename... Others>
struct type_list_pop_first<Template<Ty, Others...>> {
    using type = Template<Others...>;
};
template <typename TypeList>
using type_list_pop_first_t = typename type_list_pop_first<TypeList>::type;

template <template <typename, typename> typename Predicate, typename TypeList>
struct type_list_sort;
template <
    template <typename, typename> typename Predicate,
    template <typename...> typename Template>
struct type_list_sort<Predicate, Template<>> {
    using type = Template<>;
};
template <
    template <typename, typename> typename Predicate,
    template <typename...> typename Template, typename Ty>
struct type_list_sort<Predicate, Template<Ty>> {
    using type = Template<Ty>;
};
template <
    template <typename, typename> typename Predicate,
    template <typename...> typename Template, typename Lhs, typename Rhs>
struct type_list_sort<Predicate, Template<Lhs, Rhs>> {
    using type = std::conditional_t<
        Predicate<Lhs, Rhs>::value, Template<Lhs, Rhs>, Template<Rhs, Lhs>>;
};
template <
    template <typename, typename> typename Predicate,
    template <typename...> typename Template, typename... Tys>
struct type_list_sort<Predicate, Template<Tys...>> {
    using pivot_t     = type_list_first_t<Template<Tys...>>;
    using pop_first_t = type_list_pop_first_t<Template<Tys...>>;

    template <typename Ty>
    struct comp {
        constexpr static bool value = Predicate<Ty, pivot_t>::value;
    };

    using left = typename type_list_sort<
        Predicate, type_list_filt_t<comp, pop_first_t>>::type;
    using right = typename type_list_sort<
        Predicate, type_list_remove_in_t<left, pop_first_t>>::type;
    using type = type_list_cat_t<left, Template<pivot_t>, right>;
};
template <template <typename, typename> typename Predicate, typename TypeList>
using type_list_sort_t = typename type_list_sort<Predicate, TypeList>::type;

template <typename>
struct type_list_list_cat;
template <template <typename...> typename Template>
struct type_list_list_cat<Template<>> {
    using type = Template<>;
};
template <template <typename...> typename Template, typename... Lists>
struct type_list_list_cat<Template<Lists...>> {
    using type = Template<type_list_cat_t<Lists...>>;
};
template <typename TypeList>
using type_list_list_cat_t = typename type_list_list_cat<TypeList>::type;

template <typename Ty, typename TypeList>
struct type_list_has_same_template : std::false_type {};
template <
    template <typename...> typename Template, typename... Args1,
    typename... Args2>
struct type_list_has_same_template<Template<Args1...>, Template<Args2...>> :
    std::true_type {};
template <typename Ty, typename TypeList>
constexpr auto type_list_has_same_template_v =
    type_list_has_same_template<Ty, TypeList>::value;

template <typename TypeList>
struct type_list_combine;
template <template <typename...> typename Template>
struct type_list_combine<Template<>> {
    using type = Template<>;
};
template <
    template <typename...> typename Template, typename Ty, typename... Others>
struct type_list_combine<Template<Ty, Others...>> {
    template <typename T>
    using _has_same_template = type_list_has_same_template<Ty, T>;
    using type               = type_list_cat_t<
                      type_list_list_cat_t<
                          type_list_filt_t<_has_same_template, Template<Ty, Others...>>>,
                      typename type_list_combine<type_list_erase_if_t<
                          _has_same_template, Template<Ty, Others...>>>::type>;
};
template <typename TypeList>
using type_list_conbine_t = typename type_list_combine<TypeList>::type;

template <typename TypeList>
struct type_list_value_list_cat;
template <template <typename...> typename Template, typename... ValueLists>
struct type_list_value_list_cat<Template<ValueLists...>> {
    using type = Template<value_list_cat_t<ValueLists...>>;
};
template <typename TypeList>
using type_list_value_list_cat_t =
    typename type_list_value_list_cat<TypeList>::type;

template <template <typename> typename Predicate, typename>
struct type_list_convert;
template <
    template <typename> typename Predicate,
    template <typename...> typename Template, typename... Tys>
struct type_list_convert<Predicate, Template<Tys...>> {
    using type = Template<typename Predicate<Tys>::type...>;
};
template <template <typename> typename Predicate, typename TypeList>
using type_list_convert_t =
    typename type_list_convert<Predicate, TypeList>::type;

template <typename To, typename From>
using ignore_cvref = std::remove_cvref<To>;

template <
    template <typename...> typename Tmp, typename TypeList,
    template <typename, typename> typename Qualifier = ignore_cvref>
struct type_list_recurse_expose;
template <
    template <typename...> typename Tmp,
    template <typename...> typename Template,
    template <typename, typename> typename Qualifier>
struct type_list_recurse_expose<Tmp, Template<>, Qualifier> {
    using type = Template<>;
};
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
template <
    template <typename...> typename Tmp, typename TypeList,
    template <typename, typename> typename Qualifier = ignore_cvref>
using type_list_recurse_expose_t =
    typename type_list_recurse_expose<Tmp, TypeList, Qualifier>::type;

template <template <typename...> typename Template, typename TypeList>
struct type_list_rebind;
template <
    template <typename...> typename Template,
    template <typename...> typename Tmp, typename... Tys>
struct type_list_rebind<Template, Tmp<Tys...>> {
    using type = Template<Tys...>;
};
template <template <typename...> typename Template, typename TypeList>
using type_list_rebind_t = typename type_list_rebind<Template, TypeList>::type;

template <typename Ty, typename Tuple>
struct tuple_first;
template <
    typename Ty, template <typename...> typename Template, typename... Types>
struct tuple_first<Ty, Template<Types...>> {
    constexpr static size_t value =
        []<size_t... Is>(std::index_sequence<Is...>) {
            auto index = static_cast<size_t>(-1);
            (..., (index = (std::is_same_v<Ty, Types> ? Is : index)));
            return index;
        }(std::index_sequence_for<Types...>());
    static_assert(value < sizeof...(Types), "Type not found in tuple");
};
template <typename Ty, typename Tuple>
constexpr size_t tuple_first_v = tuple_first<Ty, Tuple>::value;

template <typename Ty, typename Tuple>
struct tuple_last;
template <
    typename Ty, template <typename...> typename Template, typename... Types>
struct tuple_last<Ty, Template<Types...>> {
    constexpr static size_t value =
        []<size_t... Is>(std::index_sequence<Is...>) {
            auto index = static_cast<size_t>(-1);
            ((index = (std::is_same_v<Ty, Types> ? Is : index)), ...);
            return index;
        }(std::index_sequence_for<Types...>());
    static_assert(value < sizeof...(Types), "Type not found in tuple");
};
template <typename Ty, typename Tuple>
constexpr size_t tuple_last_v = tuple_first<Ty, Tuple>::value;

template <typename Ty, typename... Tys>
requires(tuple_first_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr Ty& get_first(std::tuple<Tys...>& tuple) noexcept {
    return std::get<tuple_first_v<Ty, std::tuple<Tys...>>>(tuple);
}

template <typename Ty, typename... Tys>
requires(tuple_first_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr const Ty& get_first(const std::tuple<Tys...>& tuple) noexcept {
    return std::get<tuple_first_v<Ty, std::tuple<Tys...>>>(tuple);
}

template <typename Ty, typename... Tys>
requires(tuple_first_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr Ty& get_first(shared_tuple<Tys...>& tup) noexcept {
    return tup.template get<tuple_first_v<Ty, shared_tuple<Tys...>>>();
}

template <typename Ty, typename... Tys>
requires(tuple_first_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr const Ty& get_first(const shared_tuple<Tys...>& tup) noexcept {
    return tup.template get<tuple_first_v<Ty, shared_tuple<Tys...>>>();
}

template <typename Ty, typename... Tys>
requires(tuple_last_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr Ty& get_last(std::tuple<Tys...>& tuple) noexcept {
    return std::get<tuple_last_v<Ty, std::tuple<Tys...>>>(tuple);
}

template <typename Ty, typename... Tys>
requires(tuple_last_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr const Ty& get_last(const std::tuple<Tys...>& tuple) noexcept {
    return std::get<tuple_last_v<Ty, std::tuple<Tys...>>>(tuple);
}

template <typename Ty, typename... Tys>
requires(tuple_last_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr Ty& get_last(shared_tuple<Tys...>& tup) noexcept {
    return tup.template get<tuple_last_v<Ty, shared_tuple<Tys...>>>();
}

template <typename Ty, typename... Tys>
requires(tuple_last_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr const Ty& get_last(const shared_tuple<Tys...>& tup) noexcept {
    return tup.template get<tuple_last_v<Ty, shared_tuple<Tys...>>>();
}

template <typename Ty, typename Tup>
constexpr size_t _rmcvref_first = static_cast<size_t>(-1);
template <
    typename Ty, template <typename...> typename Template, typename... Tys>
constexpr size_t _rmcvref_first<Ty, Template<Tys...>> =
    []<size_t... Is>(std::index_sequence<Is...>) {
        auto index = static_cast<size_t>(sizeof...(Tys));
        (...,
         (index = std::is_same_v<std::remove_cvref_t<Tys>, Ty> ? Is : index));
        return index;
    }(std::index_sequence_for<Tys...>());

template <typename Ty, typename Tup>
requires(
    _rmcvref_first<Ty, std::remove_cvref_t<Tup>> <
    type_list_size_v<std::remove_cvref_t<Tup>>)
constexpr decltype(auto) rmcvref_first(Tup&& tup) noexcept {
    using tuple          = std::remove_cvref_t<Tup>;
    constexpr auto index = _rmcvref_first<Ty, tuple>;
    return get<index>(std::forward<Tup>(tup));
}

} // namespace neutron
