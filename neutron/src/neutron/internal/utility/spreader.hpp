#pragma once

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template <auto...>
struct value_spreader {
    explicit value_spreader() = default;
};

template <typename...>
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
using internal::value_spreader;

/**
 * @brief Template helper for passing type arguments
 *
 * @tparam typename Type to pass
 *
 * Usage:
 *   func(spread_type<int>);
 */
using internal::type_spreader;

template <auto... Candidate>
inline constexpr value_spreader<Candidate...> spread_arg{};

template <typename... Tys>
inline constexpr type_spreader<Tys...> spread_type{}; ///< Global instance

} // namespace neutron
