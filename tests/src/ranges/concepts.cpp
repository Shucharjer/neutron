#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <string>
#include <vector>
#include <neutron/ranges.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// C++20-only input iterator: no iterator_category, post-increment returns
/// void, so iterator_traits is empty for it.
class cxx20_input_iterator {
public:
    using value_type      = int;
    using difference_type = std::ptrdiff_t;

    cxx20_input_iterator() = default;
    explicit cxx20_input_iterator(const int* ptr) noexcept : ptr_(ptr) {}

    int operator*() const noexcept { return *ptr_; }
    cxx20_input_iterator& operator++() noexcept {
        ++ptr_;
        return *this;
    }
    void operator++(int) noexcept { ++ptr_; }

    friend bool operator==(
        const cxx20_input_iterator&, const cxx20_input_iterator&) = default;

private:
    const int* ptr_ = nullptr;
};

using cxx20_range = std::ranges::subrange<cxx20_input_iterator>;
static_assert(std::input_iterator<cxx20_input_iterator>);
static_assert(std::ranges::common_range<cxx20_range>);

/// Container constructible only from an iterator pair plus a tag.
struct tagged_container {
    struct tag {};
    template <typename It>
    tagged_container(It, It, tag) {}
};

/// Container with emplace_back only.
struct emplace_only {
    std::vector<int> data;
    template <typename... Args>
    int& emplace_back(Args&&... args) {
        return data.emplace_back(std::forward<Args>(args)...);
    }
};

using vec_int = std::vector<int>;
/// Non-common view: its sentinel differs from its iterator.
using take_while_view =
    decltype(std::declval<vec_int&>() | std::views::take_while([](int) {
                 return true;
             }));
/// Infinite, hence non-common, view.
using unbounded_iota = decltype(std::views::iota(0));

// ref_convertible_range
// satisfied by
static_assert(ref_convertible_range<vec_int, std::vector<long>>);
static_assert(ref_convertible_range<std::vector<char>, std::string>);
static_assert(ref_convertible_range<
              std::map<int, int>, std::vector<std::pair<int, int>>>);
// Non-range "containers" are always accepted.
static_assert(ref_convertible_range<std::vector<std::string>, int>);
// rejected: inconvertible element types
static_assert(!ref_convertible_range<std::vector<std::string>, vec_int>);
static_assert(!ref_convertible_range<vec_int, std::vector<std::string>>);

// common_range
// satisfied by
static_assert(neutron::common_range<vec_int>);
static_assert(neutron::common_range<const vec_int>);
static_assert(neutron::common_range<std::list<int>>);
static_assert(neutron::common_range<int[4]>);
static_assert(neutron::common_range<std::ranges::iota_view<int, int>>);
static_assert(neutron::common_range<std::ranges::common_view<take_while_view>>);
// rejected: sentinel differs from the iterator, or not a range
static_assert(!neutron::common_range<unbounded_iota>);
static_assert(!neutron::common_range<take_while_view>);
static_assert(!neutron::common_range<int>);
// agrees with the standard concept
static_assert(
    neutron::common_range<vec_int> == std::ranges::common_range<vec_int>);
static_assert(
    neutron::common_range<take_while_view> ==
    std::ranges::common_range<take_while_view>);

// common_constructible_range
// satisfied by
static_assert(common_constructible_range<vec_int, std::list<int>>);
static_assert(common_constructible_range<std::list<int>, vec_int>);
static_assert(common_constructible_range<vec_int, std::set<long>>);
static_assert(
    common_constructible_range<vec_int, vec_int, std::allocator<int>>);
static_assert(common_constructible_range<
              std::ranges::iota_view<int, int>, std::deque<int>>);
static_assert(common_constructible_range<
              vec_int, tagged_container, tagged_container::tag>);
// rejected: missing the extra constructor argument
static_assert(!common_constructible_range<vec_int, tagged_container>);
// rejected: not common
static_assert(!common_constructible_range<take_while_view, vec_int>);
static_assert(!common_constructible_range<unbounded_iota, vec_int>);
// rejected: no iterator_category
static_assert(!common_constructible_range<cxx20_range, vec_int>);
// rejected: not constructible from an iterator pair
static_assert(!common_constructible_range<vec_int, std::array<int, 3>>);

// internal helpers
// satisfied by
static_assert(internal::can_emplace_back<vec_int, int&>);
static_assert(internal::can_push_back<vec_int, int&>);
static_assert(internal::can_emplace_end<vec_int, int&>);
static_assert(internal::can_insert_end<vec_int, int&>);
static_assert(internal::can_emplace_back<emplace_only, int>);
static_assert(internal::can_emplace_end<std::set<int>, int>);
static_assert(internal::can_insert_end<std::set<int>, int>);
// rejected: missing member or inconvertible element
static_assert(!internal::can_push_back<emplace_only, int>);
static_assert(!internal::can_emplace_back<std::set<int>, int>);
static_assert(!internal::can_push_back<std::forward_list<int>, int>);
static_assert(!internal::can_insert_end<std::forward_list<int>, int>);
static_assert(!internal::can_push_back<vec_int, std::string>);

// constructible_appendable_range
// satisfied by
static_assert(constructible_appendable_range<vec_int, vec_int>);
static_assert(constructible_appendable_range<std::list<int>, std::deque<int>>);
static_assert(constructible_appendable_range<vec_int, std::set<int>>);
static_assert(constructible_appendable_range<std::vector<char>, std::string>);
static_assert(constructible_appendable_range<vec_int, emplace_only>);
static_assert(constructible_appendable_range<cxx20_range, vec_int>);
static_assert(constructible_appendable_range<take_while_view, vec_int>);
static_assert(
    constructible_appendable_range<vec_int, vec_int, std::allocator<int>>);
// rejected: not appendable, or not constructible from the extra argument
static_assert(!constructible_appendable_range<vec_int, std::array<int, 3>>);
static_assert(!constructible_appendable_range<vec_int, std::forward_list<int>>);
static_assert(!constructible_appendable_range<vec_int, vec_int, std::string>);

// constructible_from_iterator
// satisfied by
static_assert(constructible_from_iterator<int*, int>);
static_assert(constructible_from_iterator<const int*, int>);
static_assert(constructible_from_iterator<vec_int::iterator, int>);
static_assert(constructible_from_iterator<
              std::map<int, char>::iterator, std::pair<const int, char>>);
// rejected: value type differs, including by cv-qualification
static_assert(!constructible_from_iterator<int*, long>);
static_assert(!constructible_from_iterator<int*, const int>);

// compatible_range
// satisfied by
static_assert(compatible_range<vec_int, int>);
static_assert(compatible_range<const vec_int, int>);
static_assert(compatible_range<std::ranges::iota_view<int, int>, int>);
static_assert(
    compatible_range<std::map<int, char>, std::pair<const int, char>>);
// rejected: value type differs, or not a range
static_assert(!compatible_range<vec_int, long>);
static_assert(!compatible_range<int, int>);

/// Dispatches like ranges::to would.
template <typename Container, typename Rng>
Container build(Rng&& range) {
    if constexpr (common_constructible_range<Rng, Container>) {
        return Container(std::ranges::begin(range), std::ranges::end(range));
    } else {
        static_assert(constructible_appendable_range<Rng, Container>);
        Container result;
        for (auto&& elem : range) {
            if constexpr (internal::can_emplace_back<
                              Container, std::ranges::range_reference_t<Rng>>) {
                result.emplace_back(std::forward<decltype(elem)>(elem));
            } else {
                result.insert(result.end(), std::forward<decltype(elem)>(elem));
            }
        }
        return result;
    }
}

void test_dispatch_constructs_from_common_range() {
    vec_int source{ 3, 1, 2, 3 };

    auto list = build<std::list<int>>(source);

    const std::list<int> expected{ 3, 1, 2, 3 };
    require(list.size() == 4);
    require(std::ranges::equal(list, expected));
}

void test_dispatch_appends_from_non_common_range() {
    vec_int source{ 3, 1, 2, 3 };
    auto before_two = source | std::views::take_while([](int value) {
                          return value != 2;
                      });

    auto set = build<std::set<int>>(before_two);

    require_or_return(set.size() == 2, );
    require(*set.begin() == 1);
}

void test_dispatch_appends_from_cxx20_input_range() {
    vec_int source{ 3, 1, 2, 3 };
    cxx20_range input(
        cxx20_input_iterator(source.data()),
        cxx20_input_iterator(source.data() + source.size()));

    auto vec = build<vec_int>(input);

    require(vec == source);
}

} // namespace

int main() {
    test_dispatch_constructs_from_common_range();
    test_dispatch_appends_from_non_common_range();
    test_dispatch_appends_from_cxx20_input_range();
    return neutron_test::result();
}
