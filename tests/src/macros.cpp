// The header under test, which only provides macros.
#include <neutron/macros.hpp>
#ifdef ATOM_USES_MODULES
import std;
#endif
#include "require.hpp"

// The configuration macros are defined.
static_assert(ATOM_HAS_CXX20);
static_assert(ATOM_HAS_REFLECTION == 0 || ATOM_HAS_REFLECTION == 1);

#if !ATOM_HAS_REFLECTION
    // The registration macros of the legacy reflection are defined.
    #ifndef REFL_MEMBERS
        #error "REFL_MEMBERS is not defined"
    #endif
    #ifndef REFL_FUNCS
        #error "REFL_FUNCS is not defined"
    #endif
#endif

namespace {

ATOM_ATTRS(ATOM_NODISCARD)
constexpr int twice(int value) noexcept { return value * 2; }

void test_attribute_macros_expand() {
    constexpr int value = 21;

    require(twice(value) == 42);
}

} // namespace

int main() {
    test_attribute_macros_expand();
    return neutron_test::result();
}
