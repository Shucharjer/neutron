// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include <type_traits>

namespace neutron {

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
    requires std::is_same_v<Ty, std::remove_cvref_t<Ty>>
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

} // namespace neutron
