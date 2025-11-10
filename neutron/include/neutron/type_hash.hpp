#pragma once
#include <array>
#include <cstddef>
#include <string_view>
#include "neutron/neutron.hpp"
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

constexpr size_t hash(std::string_view string) noexcept {
    // DJB2 Hash
    // NOTE: This hash algorithm is not friendly to parallelization support
    // TODO: Another hash algrorithm, which is friendly to parallelization.

#if ATOM_VECTORIZABLE && false
    // bytes
    const size_t parallel_request = 16;

    #if defined(__AVX2__)
    const size_t group_size = 8;
    #elif defined(__SSE2__)
    const size_t group_size = 4;
    #endif
#endif

    const size_t magic_initial_value = 5381;
    const size_t magic               = 5;

    std::size_t value = magic_initial_value;
#if ATOM_VECTORIZABLE && false
    if (string.length() < group_size) {
#endif
        for (const char cha : string) {
            value = ((value << magic) + value) + cha;
        }
#if ATOM_VECTORIZABLE && false
    } else {
    }
#endif
    return value;
}

} // namespace internal

/*! @endcond */

template <typename Ty, auto Hasher = internal::hash>
requires std::is_same_v<Ty, std::remove_cvref_t<Ty>>
consteval size_t hash_of() noexcept {
    constexpr auto name = name_of<Ty>();
    return Hasher(name);
}

template <typename Ty, size_t Hash>
struct hash_pair {};

struct sorted_hash {
    template <typename, typename>
    struct less;
    template <typename Lty, size_t Lhash, typename Rty, size_t Rhash>
    struct less<hash_pair<Lty, Lhash>, hash_pair<Rty, Rhash>> {
        constexpr static bool value = Lhash < Rhash;
    };

    template <typename, typename>
    struct greater;
    template <typename Lty, size_t Lhash, typename Rty, size_t Rhash>
    struct greater<hash_pair<Lty, Lhash>, hash_pair<Rty, Rhash>> {
        constexpr static bool value = Lhash > Rhash;
    };

    template <typename>
    struct table;
    template <template <typename...> typename Template, typename... Tys>
    struct table<Template<Tys...>> {
        using hash_result = neutron::type_list<hash_pair<Tys, neutron::hash_of<Tys>()>...>;
        using sorted_type = neutron::type_list_sort_t<less, hash_result>;

        template <typename>
        struct _to_array;
        template <typename... Types, size_t... Hashes>
        struct _to_array<Template<hash_pair<Types, Hashes>...>> {
            constexpr static std::array value = { Hashes... };
        };
    };
};
template <typename TypeList, template <typename, typename> typename Pr = sorted_hash::less>
using sorted_hash_t = typename sorted_hash::table<TypeList>::sorted_type;

template <typename TypeList, template <typename, typename> typename Pr = sorted_hash::less>
NODISCARD consteval auto make_hash_array() noexcept {
    return sorted_hash::table<TypeList>::template _to_array<sorted_hash_t<TypeList>>::value;
}

} // namespace neutron
