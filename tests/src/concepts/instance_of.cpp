#include <map>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>
#include <neutron/concepts.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Variadic template.
template <typename...>
struct list {};

/// Single parameter template.
template <typename T>
struct box {};

/// Derives from an instance of `box` without being one.
template <typename T>
struct derived_box : box<T> {};

/// Alias that names `box` itself.
template <typename T>
using box_alias = box<T>;

// satisfied by
static_assert(instance_of<std::vector<int>, std::vector>);
static_assert(instance_of<std::map<int, int>, std::map>);
static_assert(instance_of<std::tuple<>, std::tuple>);
static_assert(instance_of<std::tuple<int, float>, std::tuple>);
static_assert(instance_of<std::pair<int, int>, std::pair>);
static_assert(instance_of<list<>, list>);
static_assert(instance_of<list<int, int&>, list>);
static_assert(instance_of<box<void>, box>);
static_assert(instance_of<box_alias<int>, box>);

// matches any of the templates
static_assert(instance_of<std::optional<int>, std::vector, std::optional>);
static_assert(instance_of<list<int>, box, list, std::tuple>);
static_assert(!instance_of<list<int>, box, std::tuple>);
static_assert(!instance_of<list<int>>);

// rejected
static_assert(!instance_of<int, std::vector>);
static_assert(!instance_of<void, list>);
static_assert(!instance_of<std::vector<int>, std::tuple>);
static_assert(!instance_of<std::unique_ptr<int>, std::shared_ptr>);

// rejected: derived classes, references, cv-qualified and compound types
static_assert(!instance_of<derived_box<int>, box>);
static_assert(!instance_of<const std::vector<int>, std::vector>);
static_assert(!instance_of<std::vector<int>&, std::vector>);
static_assert(!instance_of<std::vector<int>*, std::vector>);
static_assert(!instance_of<std::vector<int>[2], std::vector>);

} // namespace

int main() { return neutron_test::result(); }
