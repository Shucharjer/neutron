#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <neutron/reflection.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

// Byte-wise FNV-1a 32-bit reference.
constexpr std::uint32_t fnv1a_32(std::string_view str) noexcept {
    std::uint32_t value = 0x811c9dc5U;
    for (const char ch : str) {
        value ^= static_cast<unsigned char>(ch);
        value *= 0x01000193U;
    }
    return value;
}

constexpr std::uint64_t fnv1a_64_offset_basis = 0xcbf29ce484222325ULL;

// FNV-1a 64-bit over the little-endian bytes of each 32-bit element.
template <std::size_t N>
constexpr std::uint64_t
    fnv1a_64_le(const std::array<std::uint32_t, N>& values) noexcept {
    std::uint64_t value = fnv1a_64_offset_basis;
    for (const std::uint32_t elem : values) {
        for (unsigned shift = 0; shift < 32U; shift += 8U) {
            value ^= (elem >> shift) & 0xffU;
            value *= 0x100000001b3ULL;
        }
    }
    return value;
}

// Published FNV-1a 32-bit test vectors.
static_assert(internal::hash("") == 0x811c9dc5U);
static_assert(internal::hash("a") == 0xe40c292cU);
static_assert(internal::hash("foobar") == 0xbf9cf968U);

// Well-known FNV-1a 32-bit collisions.
static_assert(internal::hash("costarring") == internal::hash("liquid"));
static_assert(internal::hash("declinate") == internal::hash("macallums"));
static_assert(internal::hash("altarage") == internal::hash("zinke"));

// Bytes are hashed as unsigned and embedded nulls are significant.
static_assert(
    internal::hash(std::string_view{ "\xff\x80", 2 }) ==
    fnv1a_32(std::string_view{ "\xff\x80", 2 }));
static_assert(
    internal::hash(std::string_view{ "a\0b", 3 }) !=
    internal::hash(std::string_view{ "a", 1 }));
static_assert(internal::hash("ab") != internal::hash("ba"));

// hash_combine is FNV-1a 64-bit over the element bytes and is order sensitive.
static_assert(
    internal::hash_combine(std::array<std::uint32_t, 0>{}) ==
    fnv1a_64_offset_basis);
static_assert(
    internal::hash_combine(std::array<std::uint32_t, 1>{ 0U }) ==
    fnv1a_64_le(std::array<std::uint32_t, 1>{ 0U }));
static_assert(
    internal::hash_combine(
        std::array<std::uint32_t, 3>{ 0x01020304U, 0xdeadbeefU,
                                      0xffffffffU }) ==
    fnv1a_64_le(
        std::array<std::uint32_t, 3>{ 0x01020304U, 0xdeadbeefU, 0xffffffffU }));
static_assert(
    internal::hash_combine(std::array<std::uint32_t, 2>{ 1U, 2U }) !=
    internal::hash_combine(std::array<std::uint32_t, 2>{ 2U, 1U }));
static_assert(
    hash_combine(immediately, std::array<std::uint32_t, 2>{ 7U, 9U }) ==
    fnv1a_64_le(std::array<std::uint32_t, 2>{ 7U, 9U }));

// Only ranges of 32-bit unsigned hashes can be combined.
template <typename Rng>
concept internal_combinable =
    requires(const Rng& rng) { internal::hash_combine(rng); };
static_assert(internal_combinable<std::vector<std::uint32_t>>);
static_assert(!internal_combinable<std::vector<std::uint64_t>>);
static_assert(!internal_combinable<std::vector<int>>);

void test_runtime_hash_matches_reference() {
    const std::string words[] = {
        "", "a", "foobar", "neutron", "costarring", std::string(256, 'x')
    };

    for (const auto& word : words) {
        require(internal::hash(word) == fnv1a_32(word));
        require(
            internal::hash(word) == internal::hash(std::string_view{ word }));
    }
}

void test_runtime_hash_covers_every_byte_value() {
    constexpr int byte_values = 256;
    std::string bytes;
    for (int i = 0; i < byte_values; ++i) {
        bytes.push_back(static_cast<char>(i));
    }

    require(internal::hash(bytes) == fnv1a_32(bytes));
}

void test_runtime_combine_matches_reference() {
    const std::vector<std::uint32_t> values  = { internal::hash("int"),
                                                 internal::hash("char"), 0U,
                                                 0xffffffffU };
    const std::array<std::uint32_t, 4> array = { values[0], values[1],
                                                 values[2], values[3] };
    const auto expected                      = fnv1a_64_le(array);

    require(internal::hash_combine(values) == expected);
    require(hash_combine(values) == expected);
    require(hash_combine(std::span{ values }) == expected);
}

void test_runtime_combine_of_nothing_is_offset_basis() {
    require(
        hash_combine(std::vector<std::uint32_t>{}) == fnv1a_64_offset_basis);
}

} // namespace

int main() {
    test_runtime_hash_matches_reference();
    test_runtime_hash_covers_every_byte_value();
    test_runtime_combine_matches_reference();
    test_runtime_combine_of_nothing_is_offset_basis();
    return neutron_test::result();
}
