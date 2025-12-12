#pragma once
#include <array>
#include <cstdint>
#include <ranges>
#include "neutron/template_list.hpp"
#include "../src/neutron/internal/immediately.hpp"
#include "../src/neutron/internal/macros.hpp"
#include "../src/neutron/internal/reflection/hash.hpp"
#include "../src/neutron/internal/reflection/legacy/type_hash.hpp"

namespace neutron {

template <typename Ty, auto Hash>
struct hash_pair {};

struct sorted_hash {
    template <typename, typename>
    struct less;
    template <typename Lty, auto Lhash, typename Rty, auto Rhash>
    struct less<hash_pair<Lty, Lhash>, hash_pair<Rty, Rhash>> {
        constexpr static bool value = Lhash < Rhash;
    };

    template <typename, typename>
    struct greater;
    template <typename Lty, auto Lhash, typename Rty, auto Rhash>
    struct greater<hash_pair<Lty, Lhash>, hash_pair<Rty, Rhash>> {
        constexpr static bool value = Lhash > Rhash;
    };

    template <typename>
    struct table;
    template <template <typename...> typename Template, typename... Tys>
    struct table<Template<Tys...>> {
        using hash_result =
            neutron::type_list<hash_pair<Tys, neutron::hash_of<Tys>()>...>;
        using sorted_type = neutron::type_list_sort_t<less, hash_result>;

        template <typename>
        struct _to_array;
        template <typename... Types, auto... Hashes>
        struct _to_array<Template<hash_pair<Types, Hashes>...>> {
            constexpr static std::array value = { Hashes... };
        };
    };

    template <typename>
    struct expand;
    template <
        template <typename...> typename Template, typename... Tys,
        auto... Hashes>
    struct expand<Template<hash_pair<Tys, Hashes>...>> {
        using vlist = value_list<Hashes...>;
        using tlist = Template<Tys...>;
    };

    template <typename Ori, typename Sorted>
    struct _indices;
    template <
        template <typename...> typename Template, typename... Tys,
        typename Sorted>
    struct _indices<Template<Tys...>, Sorted> {
        static constexpr std::array value = { tuple_first_v<Tys, Sorted>... };
    };
};
template <
    typename TypeList,
    template <typename, typename> typename Pr = sorted_hash::less>
using sorted_list_t = typename sorted_hash::table<TypeList>::sorted_type;
template <typename SortedHashList>
using sorted_hash_t = typename sorted_hash::expand<SortedHashList>::vlist;
template <typename SortedHashList>
using sorted_type_t = typename sorted_hash::expand<SortedHashList>::tlist;
template <
    typename TypeList,
    template <typename, typename> typename Pr = sorted_hash::less>
consteval auto sort_index() noexcept {
    using sorted_list = sorted_list_t<TypeList, Pr>;
    using sorted_type = sorted_type_t<sorted_list>;
    return sorted_hash::_indices<TypeList, sorted_type>::value;
}

template <
    typename TypeList,
    template <typename, typename> typename Pr = sorted_hash::less>
NODISCARD consteval auto make_hash_array() noexcept {
    return sorted_hash::table<TypeList>::template _to_array<
        sorted_list_t<TypeList>>::value;
}

template <
    typename TypeList,
    template <typename, typename> typename Pr = sorted_hash::less>
NODISCARD consteval uint64_t make_array_hash() noexcept {
    auto array = make_hash_array<TypeList, Pr>();
    return internal::hash_combine(array);
}

template <std::ranges::range Range>
NODISCARD constexpr uint64_t hash_combine(Range&& range) noexcept {
    return internal::hash_combine(std::forward<Range>(range));
}

template <std::ranges::range Range>
NODISCARD consteval uint64_t
    hash_combine(immediately_t, const Range& range) noexcept {
    return internal::hash_combine(range);
}

} // namespace neutron
