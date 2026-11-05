#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

// A custom closure system for arbitrary (non-range) values, built as the
// header documents.

/// Base of the custom closure system.
template <typename Derived>
struct value_closure : adaptor_closure<Derived> {};

template <typename Ty>
concept value_closure_object = _adaptor_closure<Ty, value_closure>;

template <typename First, typename Second>
struct value_compose :
    _closure_compose<value_compose, value_closure, First, Second> {
    using _compose_base =
        _closure_compose<value_compose, value_closure, First, Second>;
    using _compose_base::_compose_base;
    using _compose_base::operator();
};

template <typename C1, typename C2>
value_compose(C1&&, C2&&)
    -> value_compose<std::remove_cvref_t<C1>, std::remove_cvref_t<C2>>;

template <typename Arg, typename Closure>
requires(!value_closure_object<Arg>) && value_closure_object<Closure> &&
        std::is_invocable_v<Closure, Arg>
constexpr decltype(auto) operator|(Arg&& arg, Closure&& closure) noexcept(
    std::is_nothrow_invocable_v<Closure, Arg>) {
    return std::forward<Closure>(closure)(std::forward<Arg>(arg));
}

template <value_closure_object Closure1, value_closure_object Closure2>
constexpr auto operator|(Closure1&& closure1, Closure2&& closure2) {
    return value_compose(
        std::forward<Closure1>(closure1), std::forward<Closure2>(closure2));
}

/// A second, unrelated closure system.
template <typename Derived>
struct other_closure : adaptor_closure<Derived> {};

/// Stateless closures for simple arithmetic.
struct inc_fn : value_closure<inc_fn> {
    constexpr int operator()(int value) const noexcept { return value + 1; }
};

struct twice_fn : value_closure<twice_fn> {
    constexpr int operator()(int value) const noexcept { return value * 2; }
};

/// Stateful closure adding a stored offset.
struct add_fn : value_closure<add_fn> {
    int offset;
    constexpr explicit add_fn(int value) noexcept : offset(value) {}
    constexpr int operator()(int value) const noexcept {
        return value + offset;
    }
};

/// Not noexcept.
struct throwing_fn : value_closure<throwing_fn> {
    int operator()(int value) const { return value; }
};

/// Changes the value type from int to std::string.
struct to_string_fn : value_closure<to_string_fn> {
    std::string operator()(int value) const { return std::to_string(value); }
};

/// Accepts anything with a `size()` member.
struct size_fn : value_closure<size_fn> {
    template <typename Str>
    requires requires(const Str& str) { str.size(); }
    constexpr auto operator()(const Str& str) const noexcept {
        return str.size();
    }
};

/// Belongs to the unrelated closure system.
struct foreign_fn : other_closure<foreign_fn> {
    int operator()(int value) const noexcept { return value; }
};

/// Value category tags reported by the closures below.
enum category : int {
    lvalue       = 0,
    const_lvalue = 1,
    rvalue       = 2,
    const_rvalue = 3
};

/// Encodes the closure's value category into the result: value * 4 + tag.
struct tag_fn : value_closure<tag_fn> {
    constexpr int operator()(int value) & noexcept {
        return value * 4 + lvalue;
    }
    constexpr int operator()(int value) const& noexcept {
        return value * 4 + const_lvalue;
    }
    constexpr int operator()(int value) && noexcept {
        return value * 4 + rvalue;
    }
    constexpr int operator()(int value) const&& noexcept {
        return value * 4 + const_rvalue;
    }
};

/// Reports the value category of the argument it receives.
struct arg_category_fn : value_closure<arg_category_fn> {
    template <typename Arg>
    constexpr int operator()(Arg&&) const noexcept {
        using raw = std::remove_reference_t<Arg>;
        if constexpr (std::is_lvalue_reference_v<Arg>) {
            return std::is_const_v<raw> ? const_lvalue : lvalue;
        } else {
            return std::is_const_v<raw> ? const_rvalue : rvalue;
        }
    }
};

/// Move-only closure owning its state.
struct owned_add_fn : value_closure<owned_add_fn> {
    std::unique_ptr<int> offset;
    explicit owned_add_fn(int value) : offset(std::make_unique<int>(value)) {}
    int operator()(int value) const { return value + *offset; }
};

/// Move-only closure that can only be invoked once, as an rvalue.
struct consume_fn : value_closure<consume_fn> {
    std::unique_ptr<int> offset;
    explicit consume_fn(int value) : offset(std::make_unique<int>(value)) {}
    int operator()(int value) && {
        auto owned = std::move(offset);
        return value + *owned;
    }
};

/// Accepts a move-only argument.
struct deref_fn : value_closure<deref_fn> {
    int operator()(std::unique_ptr<int> ptr) const { return *ptr; }
};

constexpr inc_fn inc{};
constexpr twice_fn twice{};

// adaptor_closure is an empty, trivial tag base.
static_assert(std::is_empty_v<adaptor_closure<inc_fn>>);
static_assert(std::is_nothrow_default_constructible_v<adaptor_closure<inc_fn>>);
static_assert(std::is_trivially_copyable_v<adaptor_closure<inc_fn>>);
static_assert(std::is_empty_v<inc_fn>);

// Only closures of the requested system satisfy _adaptor_closure.
static_assert(_adaptor_closure<inc_fn, value_closure>);
static_assert(_adaptor_closure<inc_fn&, value_closure>);
static_assert(_adaptor_closure<const inc_fn&, value_closure>);
static_assert(_adaptor_closure<inc_fn&&, value_closure>);
static_assert(_adaptor_closure<owned_add_fn, value_closure>);
static_assert(_adaptor_closure<owned_add_fn&&, value_closure>);
static_assert(!_adaptor_closure<int, value_closure>);
static_assert(!_adaptor_closure<std::string, value_closure>);
static_assert(!_adaptor_closure<foreign_fn, value_closure>);
static_assert(_adaptor_closure<foreign_fn, other_closure>);
static_assert(!_adaptor_closure<inc_fn, other_closure>);

// _adaptor_closure_for additionally requires invocability with the argument.
static_assert(_adaptor_closure_for<inc_fn, value_closure, int>);
static_assert(_adaptor_closure_for<size_fn, value_closure, std::string>);
static_assert(!_adaptor_closure_for<inc_fn, value_closure, std::string>);
static_assert(!_adaptor_closure_for<foreign_fn, value_closure, int>);

// Composition yields nested value_compose types, grouped as written.
using inc_twice = value_compose<inc_fn, twice_fn>;
static_assert(std::same_as<decltype(inc | twice), inc_twice>);
static_assert(std::same_as<decltype(inc_fn{} | twice_fn{}), inc_twice>);
static_assert(std::derived_from<inc_twice, value_closure<inc_twice>>);
static_assert(_adaptor_closure<inc_twice, value_closure>);
static_assert(std::same_as<
              decltype(inc | twice | inc), value_compose<inc_twice, inc_fn>>);
static_assert(std::same_as<
              decltype(inc | (twice | inc)),
              value_compose<inc_fn, value_compose<twice_fn, inc_fn>>>);

// Empty closures are compressed away.
static_assert(sizeof(inc_twice) == 1);
static_assert(sizeof(value_compose<add_fn, inc_fn>) == sizeof(int));
static_assert(sizeof(value_compose<inc_fn, add_fn>) == sizeof(int));

// The composition is invocable when the chained stages are.
static_assert(std::is_invocable_r_v<int, inc_twice&, int>);
static_assert(std::is_invocable_r_v<int, const inc_twice&, int>);
static_assert(std::is_invocable_r_v<int, inc_twice&&, int>);
static_assert(std::is_invocable_r_v<int, const inc_twice&&, int>);
static_assert(!std::is_invocable_v<inc_twice&, std::string>);
static_assert(!std::is_invocable_v<value_compose<to_string_fn, inc_fn>&, int>);
static_assert(std::is_invocable_r_v<
              std::size_t, value_compose<to_string_fn, size_fn>&, int>);

// noexcept follows both stages.
static_assert(std::is_nothrow_invocable_v<inc_twice&, int>);
static_assert(std::is_nothrow_invocable_v<const inc_twice&&, int>);
static_assert(
    !std::is_nothrow_invocable_v<value_compose<throwing_fn, inc_fn>&, int>);
static_assert(
    !std::is_nothrow_invocable_v<value_compose<inc_fn, throwing_fn>&, int>);

// Rvalue-only stages make the composition rvalue-only.
using consume_compose = value_compose<inc_fn, consume_fn>;
static_assert(!std::is_invocable_v<consume_compose&, int>);
static_assert(!std::is_invocable_v<const consume_compose&, int>);
static_assert(std::is_invocable_v<consume_compose&&, int>);
static_assert(!std::is_copy_constructible_v<consume_compose>);
static_assert(std::is_move_constructible_v<consume_compose>);

// Compositions are usable in constant expressions.
constexpr auto inc_then_twice = inc | twice;
static_assert(inc_then_twice(3) == 8);
static_assert((3 | inc_then_twice) == 8);
static_assert((3 | twice | inc) == 7);

constexpr bool stateful_composition_in_constexpr() {
    auto offset = add_fn(5) | twice;
    return offset(1) == 12 && (1 | offset) == 12;
}
static_assert(stateful_composition_in_constexpr());

/// Result of `tag_fn{} | tag_fn{}` applied to 0, given how each stage ran.
constexpr int tagged(category first_stage, category second_stage) noexcept {
    return first_stage * 4 + second_stage;
}

void test_value_pipes_into_closures_left_to_right() {
    require((3 | inc) == 4);
    require((3 | inc | twice) == 8);
    require((3 | twice | inc) == 7);
    require((3 | (inc | twice)) == 8);
}

void test_composition_is_associative() {
    auto left  = (inc | twice) | add_fn(10);
    auto right = inc | (twice | add_fn(10));

    require(left(1) == 14);
    require(right(1) == 14);
    require((1 | left) == (1 | right));
}

void test_compositions_compose() {
    auto both = (inc | twice) | (add_fn(1) | twice);

    require(both(1) == 10);
}

void test_value_type_may_change_along_the_pipeline() {
    auto digits = to_string_fn{} | size_fn{};

    require((12345 | digits) == 5);
    require((std::string("abc") | size_fn{}) == 3);
}

void test_stages_see_the_composition_value_category() {
    auto tags              = tag_fn{} | tag_fn{};
    const auto& const_tags = tags;

    require(tags(0) == tagged(lvalue, lvalue));
    require(const_tags(0) == tagged(const_lvalue, const_lvalue));
    require(std::move(tags)(0) == tagged(rvalue, rvalue));
    require(std::move(const_tags)(0) == tagged(const_rvalue, const_rvalue));
}

void test_piping_keeps_the_composition_value_category() {
    auto tags              = tag_fn{} | tag_fn{};
    const auto& const_tags = tags;

    require((0 | tags) == tagged(lvalue, lvalue));
    require((0 | const_tags) == tagged(const_lvalue, const_lvalue));
    require((0 | std::move(tags)) == tagged(rvalue, rvalue));
    require((0 | (tag_fn{} | tag_fn{})) == tagged(rvalue, rvalue));
}

void test_arguments_are_perfectly_forwarded() {
    auto forward     = arg_category_fn{} | inc;
    int value        = 0;
    const int cvalue = 0;

    // inc adds one to the reported category.
    require(forward(value) == lvalue + 1);
    require(forward(cvalue) == const_lvalue + 1);
    require(forward(0) == rvalue + 1);
    require(forward(std::move(cvalue)) == const_rvalue + 1);
    require((value | forward) == lvalue + 1);
    require((std::move(value) | forward) == rvalue + 1);
}

void test_move_only_argument_is_forwarded() {
    auto read = deref_fn{} | inc;

    require(read(std::make_unique<int>(41)) == 42);
    require((std::make_unique<int>(1) | read) == 2);
}

void test_stateful_composition_is_copyable() {
    auto offset = add_fn(10) | add_fn(100);
    require(offset(1) == 111);

    auto copy = offset;

    require(copy(2) == 112);
}

void test_lvalue_closures_are_copied_into_the_composition() {
    add_fn base(1);

    auto composed = base | twice;
    base.offset   = 1000;

    require(composed(1) == 4);
}

void test_move_only_closure_is_invocable_in_every_category() {
    auto owned = owned_add_fn(5) | twice;

    require(owned(1) == 12);
    require(std::as_const(owned)(1) == 12);
    require(std::move(owned)(2) == 14);
}

void test_rvalue_closures_are_moved_into_the_composition() {
    owned_add_fn first(1);

    auto moved = std::move(first) | owned_add_fn(2);

    require(first.offset == nullptr);
    require(moved(0) == 3);
    require((0 | std::move(moved)) == 3);
}

void test_self_consuming_stage_runs_as_rvalue() {
    auto consume       = inc | consume_fn(10);
    auto consume_again = inc | consume_fn(20);

    require(std::move(consume)(1) == 12);
    require((1 | std::move(consume_again)) == 22);
}

} // namespace

int main() {
    test_value_pipes_into_closures_left_to_right();
    test_composition_is_associative();
    test_compositions_compose();
    test_value_type_may_change_along_the_pipeline();
    test_stages_see_the_composition_value_category();
    test_piping_keeps_the_composition_value_category();
    test_arguments_are_perfectly_forwarded();
    test_move_only_argument_is_forwarded();
    test_stateful_composition_is_copyable();
    test_lvalue_closures_are_copied_into_the_composition();
    test_move_only_closure_is_invocable_in_every_category();
    test_rvalue_closures_are_moved_into_the_composition();
    test_self_consuming_stage_runs_as_rvalue();
    return neutron_test::result();
}
