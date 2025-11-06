#pragma once
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace neutron {

template <auto... Vals>
struct value_list {};

template <typename>
struct is_value_list : std::false_type {};
template <auto... Vals>
struct is_value_list<value_list<Vals...>> : std::true_type {};
template <typename Ty>
constexpr auto is_value_list_v = is_value_list<Ty>::value;

template <template <auto...> typename Template, typename>
struct is_specific_value_list : std::false_type {};
template <template <auto...> typename Template, auto... Vals>
struct is_specific_value_list<Template, Template<Vals...>> : std::true_type {};
template <template <auto...> typename Template, typename Ty>
constexpr bool is_specific_value_list_v = is_specific_value_list<Template, Ty>::value;

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
struct value_list_element<Index, value_list<Val, Others...>>
    : value_list_element<Index - 1, value_list<Others...>> {};
template <size_t Index, typename Ty>
constexpr inline auto value_list_element_v = value_list_element<Index, Ty>::value;

template <typename...>
struct value_list_cat;
template <>
struct value_list_cat<> {
    using type = value_list<>;
};
template <auto... Vals>
struct value_list_cat<value_list<Vals...>> {
    using type = value_list<Vals...>;
};
template <auto... Vals1, auto... Vals2>
struct value_list_cat<value_list<Vals1...>, value_list<Vals2...>> {
    using type = value_list<Vals1..., Vals2...>;
};
template <typename Vl1, typename Vl2, typename... Vls>
struct value_list_cat<Vl1, Vl2, Vls...> {
    using type = typename value_list_cat<typename value_list_cat<Vl1, Vl2>::type, Vls...>::type;
};
template <typename... Vls>
using value_list_cat_t = typename value_list_cat<Vls...>::type;

template <auto Lhs, auto Rhs>
struct is_same_value : std::bool_constant<std::is_same_v<value_list<Lhs>, value_list<Rhs>>> {};
template <auto Lhs, auto Rhs>
constexpr auto is_same_value_v = is_same_value<Lhs, Rhs>::value;

template <auto Val, typename>
struct value_list_has : std::false_type {};
template <auto Val, template <auto...> typename Template, auto... Vals>
struct value_list_has<Val, Template<Vals...>>
    : std::bool_constant<(is_same_value_v<Val, Vals> || ...)> {};
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
    template <auto...> typename List, template <typename...> typename Template, typename... Tys>
struct append_value_list<List, Template<Tys...>> {
    using type = Template<Tys..., List<>>;
};
template <template <auto...> typename List, typename Target>
using append_value_list_t = typename append_value_list<List, Target>::type;

template <template <auto> typename Predicate, typename ValList>
struct value_list_filt;
template <template <auto> typename Predicate, template <auto...> typename Template, auto... Tys>
struct value_list_filt<Predicate, Template<Tys...>> {
    template <typename Curr, auto... Remains>
    struct filt;
    template <auto... Curr>
    struct filt<Template<Curr...>> {
        using type = Template<Curr...>;
    };
    template <auto... Curr, auto Val>
    struct filt<Template<Curr...>, Val> {
        using type =
            std::conditional_t<Predicate<Val>::value, Template<Curr..., Val>, Template<Curr...>>;
    };
    template <auto... Curr, auto Val, auto... Others>
    struct filt<Template<Curr...>, Val, Others...> {
        using type = std::conditional_t<
            Predicate<Val>::value, typename filt<Template<Curr..., Val>, Others...>::type,
            typename filt<Template<Curr...>, Others...>::type>;
    };
    using type = typename filt<Template<>, Tys...>::type;
};

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

} // namespace neutron
