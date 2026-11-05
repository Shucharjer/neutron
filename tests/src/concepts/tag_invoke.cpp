#include <utility>
#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// A minimal customization point object.
inline constexpr struct get_value_t {
    template <typename Ty>
    requires tag_invocable<get_value_t, Ty>
    constexpr auto operator()(Ty&& value) const
        noexcept(nothrow_tag_invocable<get_value_t, Ty>) {
        return tag_invoke(*this, std::forward<Ty>(value));
    }
} get_value{};

/// Tag without a default constructor.
struct private_tag {
    explicit private_tag(int) {}
};

/// Customized with a noexcept `tag_invoke`.
struct nothrow_type {
    int value = 1;

    friend constexpr int
        tag_invoke(get_value_t, const nothrow_type& self) noexcept {
        return self.value;
    }
};

/// Customized with a potentially throwing `tag_invoke`.
struct throwing_type {
    int value = 2;

    friend constexpr int tag_invoke(get_value_t, const throwing_type& self) {
        return self.value;
    }
};

/// Only rvalues are customized.
struct rvalue_type {
    friend constexpr int tag_invoke(get_value_t, rvalue_type&&) noexcept {
        return 3;
    }
};

/// Customization with several arguments.
struct multi_type {
    friend constexpr int
        tag_invoke(get_value_t, multi_type, int lhs, int rhs) noexcept {
        return lhs + rhs;
    }
};

/// Customized only for the non default constructible `private_tag`.
struct private_tag_type {
    friend void tag_invoke(private_tag, private_tag_type) noexcept {}
};

/// Not customized at all.
struct plain_type {};

// nothrow customization
static_assert(tag_invocable<get_value_t, nothrow_type>);
static_assert(tag_invocable<get_value_t, nothrow_type&>);
static_assert(tag_invocable<get_value_t, const nothrow_type&>);
static_assert(nothrow_tag_invocable<get_value_t, nothrow_type>);
static_assert(nothrow_tag_invocable<get_value_t, const nothrow_type&>);

// throwing customization
static_assert(tag_invocable<get_value_t, throwing_type>);
static_assert(tag_invocable<get_value_t, throwing_type&>);
static_assert(!nothrow_tag_invocable<get_value_t, throwing_type>);
static_assert(!nothrow_tag_invocable<get_value_t, throwing_type&>);

// rvalue-only customization
static_assert(tag_invocable<get_value_t, rvalue_type>);
static_assert(tag_invocable<get_value_t, rvalue_type&&>);
static_assert(!tag_invocable<get_value_t, rvalue_type&>);
static_assert(nothrow_tag_invocable<get_value_t, rvalue_type>);
static_assert(!nothrow_tag_invocable<get_value_t, rvalue_type&>);

// customization with extra arguments, which must match in number
static_assert(tag_invocable<get_value_t, multi_type, int, int>);
static_assert(tag_invocable<get_value_t, multi_type&, long, short>);
static_assert(nothrow_tag_invocable<get_value_t, multi_type, int, int>);
static_assert(!tag_invocable<get_value_t, multi_type, int>);
static_assert(!tag_invocable<get_value_t, multi_type>);

// rejected: no matching customization
static_assert(!tag_invocable<get_value_t, plain_type>);
static_assert(!nothrow_tag_invocable<get_value_t, plain_type>);
static_assert(!tag_invocable<get_value_t, int>);
static_assert(!tag_invocable<get_value_t>);
static_assert(!tag_invocable<get_value_t, nothrow_type, int>);

// rejected: the tag must be default constructible
static_assert(!tag_invocable<private_tag, private_tag_type>);
static_assert(!nothrow_tag_invocable<private_tag, private_tag_type>);

// the CPO is constrained by the concepts
static_assert(std::is_invocable_v<get_value_t, nothrow_type>);
static_assert(std::is_nothrow_invocable_v<get_value_t, nothrow_type>);
static_assert(std::is_invocable_v<get_value_t, throwing_type&>);
static_assert(!std::is_nothrow_invocable_v<get_value_t, throwing_type&>);
static_assert(!std::is_invocable_v<get_value_t, plain_type>);
static_assert(!std::is_invocable_v<get_value_t, rvalue_type&>);

// dispatch in constant evaluation
static_assert(get_value(nothrow_type{}) == 1);
static_assert(get_value(throwing_type{}) == 2);
static_assert(get_value(rvalue_type{}) == 3);
static_assert(tag_invoke(get_value, multi_type{}, 4, 5) == 9);

void test_cpo_dispatches_on_lvalues() {
    constexpr int mutable_value = 7;
    constexpr int const_value   = 8;
    nothrow_type value{ mutable_value };
    const throwing_type cvalue{ const_value };

    require(get_value(value) == mutable_value);
    require(get_value(cvalue) == const_value);
}

void test_non_default_constructible_tag_is_usable_directly() {
    // Nothing to observe: the call has to compile and run.
    tag_invoke(private_tag{ 0 }, private_tag_type{});
}

} // namespace

int main() {
    test_cpo_dispatches_on_lvalues();
    test_non_default_constructible_tag_is_usable_directly();
    return neutron_test::result();
}
