// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <type_traits>
#include "neutron/detail/metafn/empty.hpp"

namespace neutron {

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

// value_list_filt

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

template <
    template <typename> typename Predicate, typename TypeList,
    typename TyIfEmpty>
struct type_list_filt_nempty {
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
using type_list_filt_nempty_t =
    type_list_filt_nempty<Predicate, TypeList, TyIfEmpty>::type;

} // namespace neutron
