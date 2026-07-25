#include <neutron/algorithm.hpp>
#include "require.hpp"

using namespace neutron;

int main() {

    require_or_return(min(0, 1, 2) == 0, 1);
    require_or_return(min(0, 2, 1) == 0, 1);
    require_or_return(min(1, 0, 2) == 0, 1);
    require_or_return(min(1, 2, 0) == 0, 1);
    require_or_return(min(2, 0, 1) == 0, 1);
    require_or_return(min(2, 1, 0) == 0, 1);

    require_or_return(max(0, 1, 2) == 2, 1);
    require_or_return(max(0, 2, 1) == 2, 1);
    require_or_return(max(1, 0, 2) == 2, 1);
    require_or_return(max(1, 2, 0) == 2, 1);
    require_or_return(max(2, 0, 1) == 2, 1);
    require_or_return(max(2, 1, 0) == 2, 1);

    return 0;
}
