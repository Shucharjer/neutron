// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include <concepts>
#include <ranges>
#include "neutron/detail/macros.hpp"
#include "neutron/detail/metafn/convert.hpp"
#include "neutron/detail/metafn/element.hpp"
#include "neutron/detail/metafn/make.hpp"
#include "neutron/detail/metafn/sort.hpp"
#include "neutron/detail/metafn/unique.hpp"
#include "neutron/detail/reflection/hash_t.hpp"
#include "neutron/detail/reflection/refl.hpp"
#include "neutron/detail/utility/immediately.hpp"

namespace neutron {

/**
 * @brief A helper struct taht pairs a type with a hash value.
 *
 * This struct is typically used as an intermediate representation during type
 * list sorting or hashing operations.
 *
 * @tparam Ty The original data type.
 * @tparam Hash The compile-time hash value assiciated with the type.
 */
template <typename Ty, auto Hash>
struct hash_pair {
    using type                  = Ty;
    static constexpr auto value = Hash;
};

template <typename LHashPair, typename RHashPair>
struct hash_less {
    constexpr static bool value = LHashPair::value < RHashPair::value;
};

template <typename LHashPair, typename RHashPair>
struct hash_greater {
    constexpr static bool value = LHashPair::value > RHashPair::value;
};

/**
 * @brief Generates a sorted type list based on the hash values of its elements.
 *
 * This meta-function tasks a TypeList, computes a hash for each contained type.
 * sorts them using the specified `Compare` policy, and extracts the original
 * types.
 *
 * @tparam TypeList The input type lsit (e.g., `type_list<int, float>`).
 * @tparam Hasher The hashing function/functor to apply to type names.
 * @tparam Compare The comparsion policy to use for sorting.
 *
 * @note Requires `TypeList` to satisfy `unique_type_list_t`.
 */
template <
    typename TypeList, auto Hasher = internal::hash,
    template <typename, typename> typename Compare = hash_less>
requires std::same_as<unique_type_list_t<TypeList>, TypeList>
struct hash_list;
template <
    template <typename...> typename Template, typename... Tys, auto Hasher,
    template <typename, typename> typename Compare>
struct hash_list<Template<Tys...>, Hasher, Compare> {
    using sort_result = type_list_sort_t<
        Compare, Template<hash_pair<Tys, Hasher(name_of<Tys>())>...>>;
    using type = type_list_convert_t<std::type_identity_t, sort_result>;
};

/// @brief Alias template for `hash_list::type`
template <
    typename TypeList, auto Hasher = internal::hash,
    template <typename, typename> typename Compare = hash_less>
using hash_list_t = typename hash_list<TypeList, Hasher, Compare>::type;

/**
 * @brief Extracts a value_list of hash values from a sorted TypeList.
 *
 * Sorts the input types by hash and then produces a `value_list` containing
 * the computed hash integers.
 *
 * @tparam TypeList The input type list.
 * @tparam Hasher The hashing function to use.
 * @tparam Compare The sorting comparison policy.
 */
template <
    typename TypeList, auto Hasher = internal::hash,
    template <typename, typename> typename Compare = hash_less>
struct hash_value {
    using hash_list = hash_list_t<TypeList, Hasher, Compare>;
    template <typename>
    struct _to_vlist;
    template <template <typename...> typename Template, typename... Tys>
    struct _to_vlist<Template<Tys...>> {
        using type = value_list<Hasher(name_of<Tys>())...>;
    };
    using type = typename _to_vlist<hash_list>::type;
};

/// @brief Alias template for `hash_value::type`
template <
    typename TypeList, auto Hasher = internal::hash,
    template <typename, typename> typename Compare = hash_less>
using hash_value_t = typename hash_value<TypeList, Hasher, Compare>::type;

/**
 * @brief Maps original types to their positions in the sorted hash sequence.
 *
 * This utility generates a `std::index_sequence` representing the new indices
 * of the original types after they have been sorted by their hash values.
 *
 * @tparam TypeList The input type list.
 * @tparam Hasher The hashing function to use.
 * @tparam Compare The sorting comparison policy.
 */
template <
    typename TypeList, auto Hasher = internal::hash,
    template <typename, typename> typename Compare = hash_less>
struct hash_sequence {
    using hash_list = hash_list_t<TypeList, Hasher, Compare>;

    template <typename Ty>
    static constexpr std::size_t find() noexcept {
        return []<size_t... Is>(std::index_sequence<Is...>) {
            std::size_t index = 0;
            (...,
             (index = std::is_same_v<Ty, type_list_element_t<Is, hash_list>>
                          ? Is
                          : index));
            return index;
        }(std::make_index_sequence<type_list_size_v<hash_list>>());
    }

    using type =
        std::remove_pointer_t<decltype([]<std::size_t... Is>(
                                           std::index_sequence<Is...>) {
            return static_cast<std::index_sequence<
                find<type_list_element_t<Is, TypeList>>()...>*>(nullptr);
        }(std::make_index_sequence<type_list_size_v<TypeList>>()))>;
};

/// @brief Alias template for `hash_sequence::type`
template <
    typename TypeList, auto Hasher = internal::hash,
    template <typename, typename> typename Compare = hash_less>
using hash_sequence_t = typename hash_sequence<TypeList, Hasher, Compare>::type;

/**
 * @brief Creates a compile-time array of hash values for the given TypeList.
 *
 * @tparam TypeList The input type list.
 * @tparam Hasher The hashing function.
 * @tparam Compare The sorting policy.
 * @return A `std::array` (or similar) containing the sorted hash values.
 */
template <
    typename TypeList, auto Hasher = internal::hash,
    template <typename, typename> typename Compare = hash_less>
ATOM_NODISCARD consteval auto make_hash_array() noexcept {
    return make_array<hash_value_t<TypeList>>();
}

/**
 * @brief Computes a single combined hash value for the entire TypeList.
 *
 * Generates the sorted hash array and combines all values into a single
 * `long_hash_t` fingerprint at compile time.
 *
 * @tparam TypeList The input type list.
 * @tparam Hasher The hashing function.
 * @tparam Compare The sorting policy.
 * @return The combined hash value of the sorted type list.
 */
template <
    typename TypeList, auto Hasher = internal::hash,
    template <typename, typename> typename Compare = hash_less>
ATOM_NODISCARD consteval long_hash_t make_array_hash() noexcept {
    auto array = make_hash_array<TypeList, Hasher, Compare>();
    return internal::hash_combine(array);
}

/**
 * @brief Combines a range of hash values into a single hash.
 *
 * Runtime version of the hash combination utility.
 * @param range A range of values to combine.
 * @return The combined hash value.
 */
template <std::ranges::range Range>
ATOM_NODISCARD constexpr long_hash_t hash_combine(Range&& range) noexcept {
    return internal::hash_combine(std::forward<Range>(range));
}

/**
 * @brief Combines a range of hash values into a single hash
 * (Immediate/Consteval version).
 *
 * Forces compile-time evaluation of the hash combination.
 * @param immediately Tag to trigger consteval context.
 * @param range A range of values to combine.
 * @return The combined hash value computed at compile time.
 */
template <std::ranges::range Range>
ATOM_NODISCARD consteval long_hash_t
    hash_combine(immediately_t, const Range& range) noexcept {
    return internal::hash_combine(range);
}

} // namespace neutron
