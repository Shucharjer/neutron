#include <neutron/functional.hpp>

using namespace neutron;

int main() {

    {
        delegate<void()> delegate;
        delegate.bind<[] {}>();
    }

    return 0;
}
