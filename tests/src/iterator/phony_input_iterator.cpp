#include <algorithm>
#include <cstddef>
#include <deque>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <neutron/iterator.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

template <typename Rng>
using phony = phony_input_iterator<Rng>;

/// Phony over a contiguous range of int.
using vector_phony = phony<std::vector<int>>;
/// Phony over a const range: reference and pointer become const.
using const_vector_phony = phony<const std::vector<int>>;
/// Phony over an associative range with a pair value_type.
using map_phony = phony<std::map<int, std::string>>;
/// Phony over a view whose reference is a prvalue.
using iota_phony = phony<std::ranges::iota_view<int, int>>;

/// Container type deduced by `Cnt(first, last)` from a pair of phonies.
template <template <typename...> typename Cnt, typename Rng>
using deduced_t =
    decltype(Cnt(std::declval<phony<Rng>>(), std::declval<phony<Rng>>()));

// Member types follow the range
static_assert(
    std::same_as<vector_phony::iterator_category, std::input_iterator_tag>);
static_assert(std::same_as<vector_phony::value_type, int>);
static_assert(std::same_as<vector_phony::difference_type, std::ptrdiff_t>);
static_assert(std::same_as<vector_phony::reference, int&>);
static_assert(std::same_as<vector_phony::pointer, int*>);
static_assert(std::same_as<const_vector_phony::reference, const int&>);
static_assert(std::same_as<const_vector_phony::pointer, const int*>);
static_assert(
    std::same_as<map_phony::value_type, std::pair<const int, std::string>>);
static_assert(std::same_as<iota_phony::value_type, int>);
static_assert(std::same_as<iota_phony::reference, int>);

// iterator_traits sees the member types
static_assert(std::same_as<
              std::iterator_traits<vector_phony>::iterator_category,
              std::input_iterator_tag>);
static_assert(std::same_as<
              std::iterator_traits<map_phony>::value_type,
              std::pair<const int, std::string>>);

// Rejected: it only carries types and can not really iterate
static_assert(!std::input_or_output_iterator<vector_phony>);
static_assert(!std::indirectly_readable<vector_phony>);
static_assert(!std::weakly_incrementable<vector_phony>);
static_assert(!std::equality_comparable<vector_phony>);

// Free to create: an empty, trivial tag
static_assert(std::is_trivially_default_constructible_v<vector_phony>);
static_assert(std::is_empty_v<vector_phony>);

// Its purpose: CTAD through iterator-pair deduction guides
static_assert(
    std::same_as<deduced_t<std::vector, std::list<int>>, std::vector<int>>);
static_assert(std::same_as<
              deduced_t<std::deque, std::vector<double>>, std::deque<double>>);
static_assert(
    std::same_as<deduced_t<std::list, std::set<long>>, std::list<long>>);
static_assert(
    std::same_as<
        deduced_t<std::set, std::vector<std::string>>, std::set<std::string>>);
static_assert(
    std::same_as<deduced_t<std::basic_string, std::vector<char>>, std::string>);
static_assert(std::same_as<
              deduced_t<std::vector, std::map<int, std::string>>,
              std::vector<std::pair<const int, std::string>>>);
static_assert(std::same_as<
              deduced_t<std::map, std::vector<std::pair<int, std::string>>>,
              std::map<int, std::string>>);
static_assert(std::same_as<
              deduced_t<std::vector, std::ranges::iota_view<int, int>>,
              std::vector<int>>);

// Additional constructor arguments take part in deduction too
static_assert(std::same_as<
              decltype(std::vector(
                  std::declval<vector_phony>(), std::declval<vector_phony>(),
                  std::allocator<int>{})),
              std::vector<int>>);

/// The deduced type is a real container that the actual range can fill.
void test_deduced_container_holds_range_elements() {
    std::list<int> source{ 1, 2, 3 };
    std::ranges::common_view common(source | std::views::take(2));

    deduced_t<std::vector, decltype(common)> vector(
        common.begin(), common.end());

    const std::vector expected{ 1, 2 };
    require(vector.size() == 2);
    require(std::ranges::equal(vector, expected));
}

} // namespace

int main() {
    test_deduced_container_holds_range_elements();
    return neutron_test::result();
}
