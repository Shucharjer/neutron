#include <cassert>
#include "neutron/functional.hpp"

using namespace neutron;

static int foo(int) { return 114514; }
static auto foo2 = []() { return 1919810; };

int main() {
    // delegate
    {
        delegate<int(int)> delegate1(spread_arg<&foo>);
        delegate<int()> delegate2(spread_arg<foo2>);

        assert(delegate1(int{}) == 114514);
        assert(delegate2() == 1919810);
    }
}
