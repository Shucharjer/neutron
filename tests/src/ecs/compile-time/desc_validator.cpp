// With modules, an implementation unit of neutron that imports std, see
// `internal_tests`.
#ifndef ATOM_USES_MODULES
    #include <neutron/detail/ecs/compile-time/descriptor_validator.hpp>
    #include <neutron/ecs.hpp>
#endif
#include "require.hpp"

// Placeholder: only checks that the validator header compiles.
extern "C++" int main() { return neutron_test::result(); }
