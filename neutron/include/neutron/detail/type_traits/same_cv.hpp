// IWYU pragma: private, include <neutron/type_traits.hpp>
#pragma once

namespace neutron {

/**
 * @brief Copy const-volatile qualifiers from one type to another
 */
template <typename To, typename From>
struct same_cv {
    using type = To;
};

template <typename To, typename From>
struct same_cv<To, const From> {
    using type = const To;
};

template <typename To, typename From>
struct same_cv<To, volatile From> {
    using type = volatile To;
};

template <typename To, typename From>
struct same_cv<To, const volatile From> {
    using type = const volatile To;
};

template <typename To, typename From>
using same_cv_t = typename same_cv<To, From>::type;

} // namespace neutron
