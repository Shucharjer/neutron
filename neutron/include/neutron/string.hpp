#pragma once
#include <bit>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <string_view>
#include "neutron/neutron.hpp"

namespace neutron {

// NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast,
// cppcoreguidelines-pro-bounds-pointer-arithmetic)

/*
 * @brief Find a char fastly.
 * @note We assume that the two pointer is avaliable!
 * @return The pointer to the `character`. Otherwise `last`.
 */
NODISCARD inline auto find_char(const char character, const char* first, const char* last) noexcept
    -> const char* {
#ifdef TARGET_X86
    #if defined(__AVX512F__) && defined(__AVX512BW__)
    constexpr size_t vector_width = 64; // bytes
    #else
    constexpr size_t vector_width = 32; // bytes
    #endif
#elif defined(TARGET_ARM)
    constexpr size_t vector_width = 16; // bytes
#elif defined(TARGET_RISCV)
#endif
    constexpr uintptr_t alignement = vector_width;
    constexpr uintptr_t mask       = ~(vector_width - 1);

    const char* aligned =
        std::bit_cast<const char*>((std::bit_cast<uintptr_t>(first) + alignement - 1) & mask);

    const auto* iter            = first;
    const auto* const unaligned = (aligned < last) ? aligned : last;

    for (; iter != unaligned; ++iter) {
        if (*iter == character) {
            return iter;
        }
    }

#ifdef TARGET_X86
    #if defined(__AVX512F__) && defined(__AVX512BW__)
    for (__m512i target = _mm512_set1_epi8(character); iter + vector_width <= last;
         iter += vector_width) {
        __m512i data  = _mm512_load_si512(reinterpret_cast<const __m512i*>(iter));
        uint64_t mask = _mm512_cmpeq_epi8_mask(data, target);
        if (mask != 0) {
            int offset = _mm_tzcnt_64(mask);
            return iter + offset;
        }
    }
    #elif defined(__AVX2__)
    for (__m256i target = _mm256_set1_epi8(character); iter + vector_width <= last;
         iter += vector_width) {
        __m256i data  = _mm256_load_si256(reinterpret_cast<const __m256i*>(iter));
        __m256i cmp   = _mm256_cmpeq_epi8(data, target);
        uint64_t mask = _mm256_movemask_epi8(cmp);
        if (mask != 0) {
            int offset = _mm_tzcnt_64(mask);
            return iter + offset;
        }
    }
    #endif
#elif defined(TARGET_ARM)
    const uint8x16_t target = vmovq_n_u8(static_cast<uint8_t>(character));
    for (; iter + vector_width <= last; iter += vector_width) {
        uint8x16_t data = vld1q_u8(reinterpret_cast<const uint8_t*>(iter));
        uint8x16_t cmp  = vceqq_u8(data, target);
        // convert to bitmask
        uint8_t mask[16];
        vst1q_u8(mask, cmp);
        for (int i = 0; i < 16; ++i) {
            if (mask[i]) {
                return iter + i;
            }
        }
    }
#elif defined(TARGET_RISCV)
#endif

    for (; iter != last; ++iter) {
        if (*iter == character) {
            return iter;
        }
    }
    return last;
}

NODISCARD inline auto find_char(const char character, const char* c_str, size_t size) noexcept {
    return find_char(character, c_str, c_str + size);
}

// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast,
// cppcoreguidelines-pro-bounds-pointer-arithmetic)

NODISCARD inline auto find_char(const char character, std::string_view view) noexcept {
    return find_char(character, view.cbegin(), view.cend());
}

template <std::ranges::range Rng>
requires std::same_as<char*, std::ranges::iterator_t<Rng>>
NODISCARD inline auto find_char(const char character, const Rng& range) noexcept {
    return find_char(character, std::begin(range), std::end(range));
}

} // namespace neutron
