#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Member `get` that may throw.
struct member_gettible {
    int val;
    template <std::size_t I>
    auto& get() {
        return val;
    }
    template <std::size_t I>
    const auto& get() const {
        return val;
    }
};

/// Member `get` declared noexcept.
struct nothrow_member_gettible {
    int val;
    template <std::size_t I>
    auto& get() noexcept {
        return val;
    }
    template <std::size_t I>
    const auto& get() const noexcept {
        return val;
    }
};

/// Types whose `get` is found by argument-dependent lookup.
namespace adl {

/// Free `get` that may throw.
struct adl_gettible {
    int val;
};

template <std::size_t I>
int& get(adl_gettible& obj) {
    return obj.val;
}

template <std::size_t I>
const int& get(const adl_gettible& obj) {
    return obj.val;
}

/// Free `get` declared noexcept.
struct nothrow_adl_gettible {
    int val;
};

template <std::size_t I>
int& get(nothrow_adl_gettible& obj) noexcept {
    return obj.val;
}

template <std::size_t I>
const int& get(const nothrow_adl_gettible& obj) noexcept {
    return obj.val;
}

/// Free `get` that only accepts rvalues and returns by value.
struct rvalue_adl_gettible {
    explicit rvalue_adl_gettible(int value) : val(value) {}
    int val;
};

template <std::size_t Index>
int get(rvalue_adl_gettible&& obj) noexcept {
    return obj.val;
}

} // namespace adl

using tuple_type = std::tuple<std::unique_ptr<int>>;

// Member get: noexcept is detected.
static_assert(!_get::_has_nothrow_member_get<member_gettible, 0>);
static_assert(_get::_has_nothrow_member_get<nothrow_member_gettible, 0>);

// ADL get: found, and noexcept is detected.
static_assert(_get::_has_adl_get<adl::adl_gettible, 0>);
static_assert(!_get::_has_nothrow_adl_get<adl::adl_gettible, 0>);
static_assert(_get::_has_nothrow_adl_get<adl::nothrow_adl_gettible, 0>);

// std::get: the value category of the tuple is kept.
static_assert(std::is_same_v<
              decltype(neutron::get<0>(std::declval<tuple_type&>())),
              std::unique_ptr<int>&>);
static_assert(std::is_same_v<
              decltype(neutron::get<0>(std::declval<const tuple_type&>())),
              const std::unique_ptr<int>&>);
static_assert(std::is_same_v<
              decltype(neutron::get<0>(std::declval<tuple_type>())),
              std::unique_ptr<int>&&>);
static_assert(noexcept(neutron::get<0>(std::declval<tuple_type>())));

constexpr int stored = 8;

void test_member_get_is_called() {
    require(get<0>(member_gettible{ stored }) == stored);
}

void test_nothrow_member_get_is_called() {
    require(get<0>(nothrow_member_gettible{ stored }) == stored);
}

void test_adl_get_is_called() {
    require(get<0>(adl::adl_gettible{ stored }) == stored);
}

void test_nothrow_adl_get_is_called() {
    require(get<0>(adl::nothrow_adl_gettible{ stored }) == stored);
}

void test_rvalue_only_adl_get_is_called() {
    require(get<0>(adl::rvalue_adl_gettible{ 42 }) == 42);
}

void test_std_get_moves_out_of_rvalue_tuple() {
    tuple_type tuple{ std::make_unique<int>(42) };

    auto value = neutron::get<0>(std::move(tuple));

    require_or_return(value != nullptr, );
    require(*value == 42);
    require_false(std::get<0>(tuple));
}

} // namespace

int main() {
    test_member_get_is_called();
    test_nothrow_member_get_is_called();
    test_adl_get_is_called();
    test_nothrow_adl_get_is_called();
    test_rvalue_only_adl_get_is_called();
    test_std_get_moves_out_of_rvalue_tuple();
    return neutron_test::result();
}
