#include <array>
#include <cstddef>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <sstream>
#include <vector>
#include <neutron/iterator.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Forward iterator over an int array whose operations can be made
/// potentially-throwing one at a time.
template <
    bool NoexceptInc = true, bool NoexceptAssign = true, bool NoexceptEq = true,
    bool NoexceptDeref = true>
class probe_iterator {
public:
    using value_type      = int;
    using difference_type = std::ptrdiff_t;

    probe_iterator() = default;
    explicit probe_iterator(int* ptr) noexcept : ptr_(ptr) {}

    probe_iterator(const probe_iterator&) = default;
    probe_iterator&
        operator=(const probe_iterator& that) noexcept(NoexceptAssign) {
        ptr_ = that.ptr_;
        return *this;
    }

    int& operator*() const noexcept(NoexceptDeref) { return *ptr_; }

    probe_iterator& operator++() noexcept(NoexceptInc) {
        ++ptr_;
        return *this;
    }
    probe_iterator operator++(int) noexcept(NoexceptInc) {
        auto tmp = *this;
        ++ptr_;
        return tmp;
    }

    friend bool operator==(
        const probe_iterator& lhs,
        const probe_iterator& rhs) noexcept(NoexceptEq) {
        return lhs.ptr_ == rhs.ptr_;
    }

private:
    int* ptr_ = nullptr;
};

/// Probe whose every operation is noexcept.
using nothrow_probe = probe_iterator<>;
/// Probe whose increment may throw.
using throwing_increment_probe = probe_iterator<false, true, true, true>;
/// Probe whose copy assignment may throw.
using throwing_assign_probe = probe_iterator<true, false, true, true>;
/// Probe whose equality comparison may throw.
using throwing_equality_probe = probe_iterator<true, true, false, true>;
/// Probe whose dereference may throw.
using throwing_deref_probe = probe_iterator<true, true, true, false>;
/// Probe whose every operation may throw.
using throwing_probe = probe_iterator<false, false, false, false>;

/// Value returned by pick() when the plain forward_iterator overload wins.
constexpr int picked_forward = 1;
/// Value returned by pick() when the nothrow_forward_iterator overload wins.
constexpr int picked_nothrow = 2;

/// Overload resolution probe: the nothrow overload is more constrained.
template <forward_iterator It>
constexpr int pick(It) {
    return picked_forward;
}

template <nothrow_forward_iterator It>
constexpr int pick(It) {
    return picked_nothrow;
}

// The probe really is a forward iterator in every configuration.
static_assert(std::forward_iterator<nothrow_probe>);
static_assert(std::forward_iterator<throwing_probe>);

// Re-exported standard concepts: satisfied by
static_assert(neutron::input_iterator<int*>);
static_assert(neutron::forward_iterator<int*>);
static_assert(neutron::random_access_iterator<int*>);
static_assert(neutron::contiguous_iterator<int*>);
static_assert(neutron::contiguous_iterator<std::vector<int>::iterator>);
static_assert(neutron::random_access_iterator<std::deque<int>::iterator>);
static_assert(neutron::forward_iterator<std::list<int>::iterator>);
static_assert(neutron::input_iterator<std::istream_iterator<int>>);

// Re-exported standard concepts: rejected below the iterator's real category
static_assert(!neutron::contiguous_iterator<std::deque<int>::iterator>);
static_assert(!neutron::random_access_iterator<std::list<int>::iterator>);
static_assert(!neutron::forward_iterator<std::istream_iterator<int>>);
static_assert(!neutron::input_iterator<std::ostream_iterator<int>>);
static_assert(!neutron::input_iterator<int>);

// nothrow_forward_iterator: satisfied by pointers and standard iterators
static_assert(nothrow_forward_iterator<int*>);
static_assert(nothrow_forward_iterator<const int*>);
static_assert(nothrow_forward_iterator<std::array<int, 4>::iterator>);
static_assert(nothrow_forward_iterator<std::vector<int>::iterator>);
static_assert(nothrow_forward_iterator<std::vector<int>::const_iterator>);
// Node-based iterators are only guaranteed noexcept by libstdc++.
#if defined(__GLIBCXX__)
static_assert(nothrow_forward_iterator<std::deque<int>::iterator>);
static_assert(nothrow_forward_iterator<std::list<int>::iterator>);
static_assert(nothrow_forward_iterator<std::forward_list<int>::iterator>);
#endif

// nothrow_forward_iterator: satisfied when every operation is noexcept
static_assert(nothrow_forward_iterator<nothrow_probe>);

// nothrow_forward_iterator: rejected if any single operation may throw
static_assert(!nothrow_forward_iterator<throwing_increment_probe>);
static_assert(!nothrow_forward_iterator<throwing_assign_probe>);
static_assert(!nothrow_forward_iterator<throwing_equality_probe>);
static_assert(!nothrow_forward_iterator<throwing_deref_probe>);

// nothrow_forward_iterator: rejected for non-forward iterators
static_assert(!nothrow_forward_iterator<std::istream_iterator<int>>);
static_assert(!nothrow_forward_iterator<void*>);
static_assert(!nothrow_forward_iterator<int>);

void test_nothrow_overload_is_preferred() {
    std::array<int, 3> array{ 1, 2, 3 };
    int value = 0;

    require(pick(array.data()) == picked_nothrow);
    require(pick(nothrow_probe(&value)) == picked_nothrow);
    require(pick(throwing_increment_probe(&value)) == picked_forward);
}

void test_probe_traverses_array() {
    constexpr std::ptrdiff_t size = 4;
    constexpr int expected_sum    = 1 + 2 + 3 + 4;
    std::array<int, size> array{ 1, 2, 3, 4 };
    nothrow_probe first(array.data());
    nothrow_probe last(array.data() + array.size());

    int sum = 0;
    for (auto it = first; it != last; ++it) {
        sum += *it;
    }

    require(std::distance(first, last) == size);
    require(sum == expected_sum);
}

} // namespace

int main() {
    test_nothrow_overload_is_preferred();
    test_probe_traverses_array();
    return neutron_test::result();
}
