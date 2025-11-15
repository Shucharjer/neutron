#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include "neutron/internal/macros.hpp"
#include "neutron/template_list.hpp"

namespace neutron {

/**
 * @brief Get the name of a type.
 *
 */
template <typename Ty>
[[nodiscard]] consteval std::string_view name_of() noexcept {
#ifdef _MSC_VER
    constexpr std::string_view funcname = __FUNCSIG__;
#else
    constexpr std::string_view funcname = __PRETTY_FUNCTION__;
#endif

#ifdef __clang__
    auto split = funcname.substr(0, funcname.size() - 1);
    return split.substr(split.find_last_of(' ') + 1);
#elif defined(__GNUC__)
    auto split = funcname.substr(77);
    return split.substr(0, split.size() - 50);
#elif defined(_MSC_VER)
    auto split = funcname.substr(110);
    split      = split.substr(split.find_first_of(' ') + 1);
    return split.substr(0, split.size() - 7);
#else
    static_assert(false, "Unsupportted compiler");
#endif
}

/*! @cond TURN_OFF_DOXYGEN */

namespace internal {

constexpr uint64_t hash(std::string_view string) noexcept {
    // fnv1a
    constexpr uint64_t magic = 0xcbf29ce484222325;
    constexpr uint64_t prime = 0x100000001b3;

    uint64_t value = magic;
    for (char ch : string) {
        value = value ^ ch;
        value *= prime;
    }

    return value;
}

} // namespace internal

/*! @endcond */

template <typename Ty, auto Hasher = internal::hash>
requires std::is_same_v<Ty, std::remove_cvref_t<Ty>>
consteval auto hash_of() noexcept {
    constexpr auto name = name_of<Ty>();
    return Hasher(name);
}

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
