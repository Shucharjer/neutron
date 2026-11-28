#ifdef ATOM_USES_MODULES
import std;
import neutron;
#else
    #include <neutron/functional.hpp>
#endif
#include "require.hpp"

using namespace neutron;

namespace {

void test_delegate_binds_stateless_lambda() {
    delegate<void()> delegate;

    delegate.bind<[] {
    }>();
}

} // namespace

int main() {
    test_delegate_binds_stateless_lambda();
    return neutron_test::result();
}
