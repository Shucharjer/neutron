// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#include <type_traits>

namespace neutron {

/**
 * @brief Concept for public member pair types
 *
 * Requires first/second members and first_type/second_type typedefs
 */
template <typename Ty>
concept public_pair = requires(const Ty& val) {
    typename Ty::first_type;
    typename Ty::second_type;
    val.first;
    val.second;
    requires std::is_member_object_pointer_v<decltype(&Ty::first)>;
    requires std::is_member_object_pointer_v<decltype(&Ty::second)>;
};

/**
 * @brief Concept for private member pair types
 *
 * Requires first()/second() methods and first_type/second_type typedefs
 */
template <typename Ty>
concept private_pair = requires(const Ty& val) {
    typename Ty::first_type;
    typename Ty::second_type;
    val.first();
    val.second();
};

/**
 * @brief General pair concept
 *
 * Matches either public or private pair types
 */
template <typename Ty>
concept pair = public_pair<Ty> || private_pair<Ty>;

} // namespace neutron
