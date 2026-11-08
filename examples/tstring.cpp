// Use string literals as template arguments.
#include <string_view>
#include <type_traits>
#include <neutron/print.hpp>
#include <neutron/string.hpp>

/// A value tagged with a compile-time name.
template <neutron::tstring Name, typename Ty>
struct field {
    static constexpr std::string_view name = Name.view();
    Ty value;
};

using health = field<"health", float>;
using mana   = field<"mana", float>;

// Different names give different types.
static_assert(!std::is_same_v<health, mana>);
static_assert(health::name == "health");

int main() {
    const health hp{ 87.5F };
    neutron::println("{} = {}", health::name, hp.value);
}
