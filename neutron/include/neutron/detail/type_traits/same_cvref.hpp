// IWYU pragma: private, include "neutron/type_traits.hpp"
#pragma once

namespace neutron {

/**
 * @brief Copy both cv and reference qualifiers
 */
template <typename To, typename From>
struct same_cvref {
    using type = To;
};

template <typename To, typename From>
struct same_cvref<To, From&> {
    using type = To&;
};

template <typename To, typename From>
struct same_cvref<To, From&&> {
    using type = To&&;
};

template <typename To, typename From>
struct same_cvref<To, const From> {
    using type = const To;
};

template <typename To, typename From>
struct same_cvref<To, const From&> {
    using type = const To&;
};

template <typename To, typename From>
struct same_cvref<To, const From&&> {
    using type = const To&&;
};

template <typename To, typename From>
struct same_cvref<To, volatile From> {
    using type = const To;
};

template <typename To, typename From>
struct same_cvref<To, volatile From&> {
    using type = const To&;
};

template <typename To, typename From>
struct same_cvref<To, volatile From&&> {
    using type = const To&&;
};

template <typename To, typename From>
struct same_cvref<To, const volatile From> {
    using type = const To;
};

template <typename To, typename From>
struct same_cvref<To, const volatile From&> {
    using type = const To&;
};

template <typename To, typename From>
struct same_cvref<To, const volatile From&&> {
    using type = const To&&;
};

template <typename To, typename From>
using same_cvref_t = typename same_cvref<To, From>::type;

} // namespace neutron
