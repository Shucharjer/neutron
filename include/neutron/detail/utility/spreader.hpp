// IWYU pragma: private, include <neutron/utility.hpp>
#pragma once
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

ATOM_EXPORT template <auto...>
struct value_spreader {
    explicit value_spreader() = default;
};

ATOM_EXPORT template <typename...>
struct type_spreader {
    explicit type_spreader() = default;
};

} // namespace internal
/*! @endcond */

/**
 * @brief Template helper for passing non-type template arguments
 *
 * @tparam auto Argument value to pass
 *
 * Usage:
 *   func(spread_arg<42>);
 */
ATOM_EXPORT using internal::value_spreader;

/**
 * @brief Template helper for passing type arguments
 *
 * @tparam typename Type to pass
 *
 * Usage:
 *   func(spread_type<int>);
 */
ATOM_EXPORT using internal::type_spreader;

ATOM_EXPORT template <auto... Candidate>
inline constexpr value_spreader<Candidate...> spread_arg{};

ATOM_EXPORT template <typename... Tys>
inline constexpr type_spreader<Tys...> spread_type{}; ///< Global instance

} // namespace neutron
