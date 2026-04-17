// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include <concepts>
#include <cstdint>
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
template <
    typename TypeList, auto Hasher = internal::hash,
    template <typename, typename> typename Compare = hash_less>
using hash_list_t = typename hash_list<TypeList, Hasher, Compare>::type;

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
template <
    typename TypeList, auto Hasher = internal::hash,
    template <typename, typename> typename Compare = hash_less>
using hash_value_t = typename hash_value<TypeList, Hasher, Compare>::type;

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
template <
    typename TypeList, auto Hasher = internal::hash,
    template <typename, typename> typename Compare = hash_less>
using hash_sequence_t = typename hash_sequence<TypeList, Hasher, Compare>::type;

template <
    typename TypeList, auto Hasher = internal::hash,
    template <typename, typename> typename Compare = hash_less>
ATOM_NODISCARD consteval auto make_hash_array() noexcept {
    return make_array<hash_value_t<TypeList>>();
}

template <
    typename TypeList, auto Hasher = internal::hash,
    template <typename, typename> typename Compare = hash_less>
ATOM_NODISCARD consteval long_hash_t make_array_hash() noexcept {
    auto array = make_hash_array<TypeList, Hasher, Compare>();
    return internal::hash_combine(array);
}

template <std::ranges::range Range>
ATOM_NODISCARD constexpr long_hash_t hash_combine(Range&& range) noexcept {
    return internal::hash_combine(std::forward<Range>(range));
}

template <std::ranges::range Range>
ATOM_NODISCARD consteval long_hash_t
    hash_combine(immediately_t, const Range& range) noexcept {
    return internal::hash_combine(range);
}

} // namespace neutron
