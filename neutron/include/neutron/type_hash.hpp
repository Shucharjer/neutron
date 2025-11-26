#pragma once
#include <array>
#include <cstddef>
#include "neutron/template_list.hpp"
#include "../src/neutron/internal/macros.hpp"
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
NODISCARD consteval auto make_hash_array() noexcept {
    return sorted_hash::table<TypeList>::template _to_array<
        sorted_list_t<TypeList>>::value;
}

} // namespace neutron
