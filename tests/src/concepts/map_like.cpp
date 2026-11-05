#include <map>
#include <ranges>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>
#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

using map_t = std::map<int, float>;

/// Map-shaped elements in a container that is not associative.
using kv_vec_t = std::vector<std::pair<const int, float>>;

/// Has key_type/mapped_type but is not a range.
struct not_range {
    using key_type    = int;
    using mapped_type = int;
};

// satisfied by: associative containers and references to them
static_assert(map_like<map_t>);
static_assert(map_like<std::multimap<int, int>>);
static_assert(map_like<std::unordered_map<int, int>>);
static_assert(map_like<std::unordered_multimap<int, int>>);
static_assert(map_like<map_t&>);
static_assert(map_like<const map_t&>);
static_assert(map_like<map_t&&>);

// satisfied by: views over pairs with a const first element
static_assert(map_like<std::ranges::ref_view<map_t>>);
static_assert(map_like<decltype(std::views::all(std::declval<map_t&>()))>);
static_assert(map_like<std::ranges::subrange<kv_vec_t::iterator>>);
static_assert(map_like<std::ranges::ref_view<kv_vec_t>>);

// rejected: non-view ranges of pairs
static_assert(!map_like<kv_vec_t>);
static_assert(!map_like<std::vector<std::pair<int, int>>>);

// rejected: views over pairs with a mutable first element
static_assert(
    !map_like<std::ranges::ref_view<std::vector<std::pair<int, int>>>>);

// rejected: views over non-pairs
static_assert(!map_like<std::ranges::ref_view<std::vector<int>>>);
static_assert(!map_like<std::ranges::empty_view<int>>);
static_assert(!map_like<decltype(std::views::keys(std::declval<map_t&>()))>);

// rejected: other types
static_assert(!map_like<std::set<int>>);
static_assert(!map_like<std::vector<int>>);
static_assert(!map_like<not_range>);
static_assert(!map_like<int>);
static_assert(!map_like<int[3]>);
static_assert(!map_like<std::pair<const int, int>>);

} // namespace

int main() { return neutron_test::result(); }
