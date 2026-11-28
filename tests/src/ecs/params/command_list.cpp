#ifdef ATOM_USES_MODULES
import std;
import neutron;
#else
    #include <neutron/ecs.hpp>
#endif
#include "require.hpp"

// Placeholder: only checks that the ECS header compiles.
int main() { return neutron_test::result(); }
