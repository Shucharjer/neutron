#include <map>
#include <ranges>
#include <type_traits>
#include <vector>
#include <neutron/ranges.hpp>
#include "require.hpp"

// Declarations added to neutron::ranges::views are visible through the alias.
namespace neutron::ranges::views {
namespace {
constexpr int alias_probe = 42;
} // namespace
} // namespace neutron::ranges::views

namespace {

namespace nv  = neutron::views;
namespace nrv = neutron::ranges::views;

/// Address comparison through a function to avoid -Wtautological-compare.
template <typename Ty>
constexpr bool same_object(const Ty& lhs, const Ty& rhs) noexcept {
    return &lhs == &rhs;
}

static_assert(nv::alias_probe == 42);
static_assert(same_object(nv::alias_probe, nrv::alias_probe));

// The alias names the same entities, not copies.
static_assert(same_object(nv::keys, nrv::keys));
static_assert(same_object(nv::values, nrv::values));
static_assert(same_object(nv::elements<2>, nrv::elements<2>));
static_assert(std::same_as<
              nv::elements_view<
                  std::ranges::ref_view<std::vector<std::pair<int, int>>>, 0>,
              nrv::elements_view<
                  std::ranges::ref_view<std::vector<std::pair<int, int>>>, 0>>);
static_assert(std::same_as<nv::element_fn<1>, nrv::element_fn<1>>);

void test_alias_adaptor_iterates_keys() {
    std::map<int, int> map{
        { 1, 10 },
        { 2, 20 }
    };

    auto keys = map | nv::keys;
    static_assert(std::same_as<decltype(keys), decltype(map | nrv::keys)>);
    int sum = 0;
    for (const int key : keys) {
        sum += key;
    }

    constexpr int sum_of_keys = 1 + 2;
    require(sum == sum_of_keys);
}

} // namespace

int main() {
    test_alias_adaptor_iterates_keys();
    return neutron_test::result();
}
