#include <tuple>
#include <neutron/concepts.hpp>
#include <neutron/metafn.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// The tag being looked for.
struct tag {};

/// A tag that must not match `tag`.
struct other_tag {};

/// User-defined type list whose first parameter is the tag.
template <typename Tag, typename... Args>
struct custom_type_list {};

/// User-defined value list whose first parameter is the tag.
template <typename Tag, auto... Vals>
struct custom_value_list {};

// type_list_with_tag: satisfied by
static_assert(type_list_with_tag<tagged_type_list<tag>, tag>);
static_assert(type_list_with_tag<tagged_type_list<tag, int, float>, tag>);
static_assert(type_list_with_tag<custom_type_list<tag, int>, tag>);

// type_list_with_tag: rejected
static_assert(!type_list_with_tag<tagged_type_list<other_tag, int>, tag>);
static_assert(!type_list_with_tag<tagged_value_list<tag, 1>, tag>);
static_assert(!type_list_with_tag<tag, tag>);
static_assert(!type_list_with_tag<int, tag>);

// type_list_with_tag: rejected: references and cv-qualified types
static_assert(!type_list_with_tag<const tagged_type_list<tag>, tag>);
static_assert(!type_list_with_tag<tagged_type_list<tag>&, tag>);

// value_list_with_tag: satisfied by
static_assert(value_list_with_tag<tagged_value_list<tag>, tag>);
static_assert(value_list_with_tag<tagged_value_list<tag, 1, 'c'>, tag>);
static_assert(value_list_with_tag<custom_value_list<tag, 1>, tag>);

// value_list_with_tag: rejected
static_assert(!value_list_with_tag<tagged_value_list<other_tag, 1>, tag>);
static_assert(!value_list_with_tag<tagged_type_list<tag, int>, tag>);
static_assert(!value_list_with_tag<value_list<1, 2>, tag>);
static_assert(!value_list_with_tag<tag, tag>);
static_assert(!value_list_with_tag<const tagged_value_list<tag>, tag>);

// list_with_tag: satisfied by either kind, with the tag first
static_assert(list_with_tag<tagged_type_list<tag, int>, tag>);
static_assert(list_with_tag<tagged_value_list<tag, 1>, tag>);
static_assert(list_with_tag<custom_type_list<tag>, tag>);
static_assert(list_with_tag<custom_value_list<tag, 2>, tag>);
static_assert(list_with_tag<std::tuple<tag, int>, tag>);

// list_with_tag: rejected
static_assert(!list_with_tag<std::tuple<int, tag>, tag>);
static_assert(!list_with_tag<tagged_type_list<int, tag>, tag>);
static_assert(!list_with_tag<tagged_type_list<other_tag>, tag>);
static_assert(!list_with_tag<tagged_value_list<other_tag, 1>, tag>);
static_assert(!list_with_tag<type_list<>, tag>);
static_assert(!list_with_tag<value_list<1>, tag>);
static_assert(!list_with_tag<int, tag>);
static_assert(!list_with_tag<tagged_type_list<tag>&, tag>);

} // namespace

int main() { return neutron_test::result(); }
