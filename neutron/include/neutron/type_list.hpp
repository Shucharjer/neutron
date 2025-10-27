#pragma once
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace neutron {

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
constexpr static auto tuple_list_size_v = type_list_size<Ty>::value;

template <std::size_t Index, typename TypeList>
struct type_list_element;
template <template <typename...> typename Template, typename Ty, typename... Args>
struct type_list_element<0, Template<Ty, Args...>> {
    using type = Ty;
};
template <
    std::size_t Index, template <typename...> typename Template, typename Ty, typename... Args>
struct type_list_element<Index, Template<Ty, Args...>> {
    using type = type_list_element<Index - 1, Args...>;
};
template <std::size_t Index, typename TypeList>
using type_list_element_t = typename type_list_element<Index, TypeList>::type;

template <typename... Tls>
struct type_list_cat;
template <template <typename...> typename Template, typename... Tys>
struct type_list_cat<Template<Tys...>> {
    using type = Template<Tys...>;
};
template <template <typename...> typename Template, typename... Tys1, typename... Tys2>
struct type_list_cat<Template<Tys1...>, Template<Tys2...>> {
    using type = Template<Tys1..., Tys2...>;
};
template <typename Tl1, typename Tl2, typename... Tls>
struct type_list_cat<Tl1, Tl2, Tls...> {
    using type = typename type_list_cat<typename type_list_cat<Tl1, Tl2>::type, Tls...>::type;
};
template <typename... Tls>
using type_list_cat_t = typename type_list_cat<Tls...>::type;

template <typename, typename>
struct type_list_has : std::false_type {};
template <typename Ty, template <typename...> typename Template, typename... Types>
struct type_list_has<Ty, Template<Types...>> {
    constexpr static bool value = (std::is_same_v<Ty, Types> || ...);
};
template <typename Ty, typename Tuple>
constexpr bool type_list_has_v = type_list_has<Ty, Tuple>::value;

template <typename>
struct empty_type_list;
template <template <typename...>typename Template, typename... Tys>
struct empty_type_list<Template<Tys...>> {
    using type = Template<>;
};
template <typename TypeList>
using empty_type_list_t= typename empty_type_list<TypeList>::type;

template <typename TypeList, typename = empty_type_list_t<TypeList>>
struct unique_type_list;
template <template <typename...> typename Template, typename Current>
struct unique_type_list<Template<>, Current> {
    using type = Current;
};
template <template <typename...> typename Template, typename Ty, typename... Others, typename Prev>
struct unique_type_list<Template<Ty, Others...>, Prev> {
    using current_list = std::conditional_t<type_list_has_v<Ty, Prev>, Template<>, Template<Ty>>;
    using type = unique_type_list<Template<Others...>, type_list_cat_t<Prev, current_list>>::type;
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

template <typename Ty>
struct is_empty_template : std::false_type {};
template <template <typename...> typename Template>
struct is_empty_template<Template<>> : std::true_type {};
template <template <auto...> typename Template>
struct is_empty_template<Template<>> : std::true_type {};
template <typename Ty>
constexpr auto is_empty_template_v = is_empty_template<Ty>::value;

template <template <typename...> typename List, typename>
struct append_type_list;
template <
    template <typename...> typename List, template <typename...> typename Template, typename... Tys>
struct append_type_list<List, Template<Tys...>> {
    using type = Template<Tys..., List<>>;
};
template <template <typename...> typename List, typename Target>
using append_type_list_t = typename append_type_list<List, Target>::type;

/**
 * @class type_list_filt
 * @brief Filter the types in the list to only those for which the predicate is true.
 */
template <template <typename> typename Predicate, typename Template>
struct type_list_filt;
template <
    template <typename> typename Predicate, template <typename...> typename Template,
    typename... Tys>
struct type_list_filt<Predicate, Template<Tys...>> {
    template <typename Curr, typename... Remains>
    struct filt;
    template <typename... Curr>
    struct filt<Template<Curr...>> {
        using type = Template<Curr...>;
    };
    template <typename... Curr, typename Ty>
    struct filt<Template<Curr...>, Ty> {
        using type =
            std::conditional_t<Predicate<Ty>::value, Template<Curr..., Ty>, Template<Curr...>>;
    };
    template <typename... Curr, typename Ty, typename... Others>
    struct filt<Template<Curr...>, Ty, Others...> {
        using type = std::conditional_t<
            Predicate<Ty>::value, typename filt<Template<Curr..., Ty>, Others...>::type,
            typename filt<Template<Curr...>, Others...>::type>;
    };
    using type = typename filt<Template<>, Tys...>::type;
};
template <template <typename> typename Predicate, typename Template>
using type_list_filt_t = typename type_list_filt<Predicate, Template>::type;

template <typename>
struct type_list_first;
template <template <typename...> typename Template, typename Ty, typename... Others>
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
template <template <typename...> typename Template, typename Ty, typename... Others>
struct type_list_last<Template<Ty, Others...>> {
    using type = typename type_list_last<Others...>::type;
};
template <typename Tl>
using type_list_last_t = typename type_list_last<Tl>::type;

template <template <typename...> typename TemplateIfEmpty, typename TypeList>
struct type_list_filt_nvoid {
    template <typename T>
    using _is_template = is_specific_type_list<TemplateIfEmpty, T>;
    using type         = std::remove_pointer_t<decltype([] {
        using filted_type = type_list_filt_t<_is_template, TypeList>;
        if constexpr (is_empty_template_v<filted_type>) {
            return static_cast<TemplateIfEmpty<>*>(nullptr);
        } else {
            return static_cast<type_list_first_t<filted_type>*>(nullptr);
        }
    }())>;
};
template <template <typename...> typename TemplateIfEmpty, typename TypeList>
using type_list_filt_nvoid_t = type_list_filt_nvoid<TemplateIfEmpty, TypeList>::type;

template <typename Template, typename Old, typename New>
struct type_list_substitute;
template <template <typename...> typename Template, typename... Tys, typename Old, typename New>
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
            std::is_same_v<Ty, Old>, Template<Curr..., New>, Template<Curr..., Ty>>;
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
using type_list_substitute_t = typename type_list_substitute<Template, Old, New>::type;

template <template <typename...> typename Template, typename Ty, typename Target>
struct insert_type_list_inplace {
    template <typename T>
    using _is_the_type_list = is_specific_type_list<Template, T>;
    using type              = std::remove_pointer_t<decltype([]() {
        using filted_type = type_list_filt_t<_is_the_type_list, Target>;
        if constexpr (is_empty_template<filted_type>::value) {
            using fixed_type   = typename append_type_list<Template, Target>::type;
            using filted_type  = type_list_filt_t<_is_the_type_list, fixed_type>;
            using current_type = type_list_first_t<filted_type>;
            using result       = type_list_substitute_t<
                                   fixed_type, current_type, type_list_cat_t<current_type, Template<Ty>>>;
            return static_cast<result*>(nullptr);
        } else {
            using current_type = type_list_first_t<filted_type>;
            using result       = type_list_substitute_t<
                                   Target, current_type, type_list_cat_t<current_type, Template<Ty>>>;
            return static_cast<result*>(nullptr);
        }
    }())>;
};
template <template <typename...> typename Template, typename Ty, typename Target>
using insert_type_list_inplace_t = typename insert_type_list_inplace<Template, Ty, Target>::type;

template <typename Ty, typename List>
struct type_list_remove;
template <typename Ty, template <typename...> typename Template, typename... Tys>
struct type_list_remove<Ty, Template<Tys...>> {
    template <typename T>
    using predicate_type = std::negation<std::is_same<Ty, T>>;
    using type           = type_list_filt_t<predicate_type, Template<Tys...>>;
};
template <typename Ty, typename List>
using type_list_remove_t = typename type_list_remove<Ty, List>::type;

template <typename List, typename Ty>
struct in_type_list;
template <template <typename...> typename Template, typename... Tys, typename Ty>
struct in_type_list<Template<Tys...>, Ty> {
    constexpr static bool value = (std::is_same_v<Tys, Ty> || ...);
};
template <typename List, typename Ty>
constexpr auto in_type_list_v = in_type_list<List, Ty>::value;

template <typename List, typename Target>
struct type_list_remove_in;
template <template <typename...> typename ListTemplate, typename... ListTys, typename Target>
struct type_list_remove_in<ListTemplate<ListTys...>, Target> {
    template <typename T>
    using predicate_type = std::negation<in_type_list<ListTemplate<ListTys...>, T>>;
    using type           = type_list_filt_t<predicate_type, Target>;
};
template <typename List, typename Target>
using type_list_remove_in_t = typename type_list_remove_in<List, Target>::type;

template <typename TypeList>
struct type_list_pop_first;
template <template <typename...> typename Template, typename Ty, typename... Others>
struct type_list_pop_first<Template<Ty, Others...>> {
    using type = Template<Others...>;
};
template <typename TypeList>
using type_list_pop_first_t = typename type_list_pop_first<TypeList>::type;

template <template <typename, typename> typename Predicate, typename TypeList>
struct type_list_sort;
template <
    template <typename, typename> typename Predicate, template <typename...> typename Template>
struct type_list_sort<Predicate, Template<>> {
    using type = Template<>;
};
template <
    template <typename, typename> typename Predicate, template <typename...> typename Template,
    typename Ty>
struct type_list_sort<Predicate, Template<Ty>> {
    using type = Template<Ty>;
};
template <
    template <typename, typename> typename Predicate, template <typename...> typename Template,
    typename Lhs, typename Rhs>
struct type_list_sort<Predicate, Template<Lhs, Rhs>> {
    using type =
        std::conditional_t<Predicate<Lhs, Rhs>::value, Template<Lhs, Rhs>, Template<Rhs, Lhs>>;
};
template <
    template <typename, typename> typename Predicate, template <typename...> typename Template,
    typename... Tys>
struct type_list_sort<Predicate, Template<Tys...>> {
    using pivot_t     = type_list_first_t<Template<Tys...>>;
    using pop_first_t = type_list_pop_first_t<Template<Tys...>>;

    template <typename Ty>
    struct comp {
        constexpr static bool value = Predicate<Ty, pivot_t>::value;
    };

    using left = typename type_list_sort<Predicate, type_list_filt_t<comp, pop_first_t>>::type;
    using right =
        typename type_list_sort<Predicate, type_list_remove_in_t<left, pop_first_t>>::type;
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

template <template <typename> typename Predicate, typename>
struct type_list_convert;
template <
    template <typename> typename Predicate, template <typename...> typename Template,
    typename... Tys>
struct type_list_convert<Predicate, Template<Tys...>> {
    using type = Template<typename Predicate<Tys>::type...>;
};
template <template <typename> typename Predicate, typename TypeList>
using type_list_convert_t = typename type_list_convert<Predicate, TypeList>::type;

template <typename Ty, typename Tuple>
struct tuple_first;
template <typename Ty, typename... Types>
struct tuple_first<Ty, std::tuple<Types...>> {
    constexpr static size_t value = []<size_t... Is>(std::index_sequence<Is...>) {
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
template <typename Ty, typename... Types>
struct tuple_last<Ty, std::tuple<Types...>> {
    constexpr static size_t value = []<size_t... Is>(std::index_sequence<Is...>) {
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
requires(tuple_last_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr Ty& get_last(std::tuple<Tys...>& tuple) noexcept {
    return std::get<tuple_last_v<Ty, std::tuple<Tys...>>>(tuple);
}

template <typename Ty, typename... Tys>
requires(tuple_last_v<Ty, std::tuple<Tys...>> != static_cast<size_t>(-1))
constexpr const Ty& get_last(const std::tuple<Tys...>& tuple) noexcept {
    return std::get<tuple_last_v<Ty, std::tuple<Tys...>>>(tuple);
}

} // namespace neutron
