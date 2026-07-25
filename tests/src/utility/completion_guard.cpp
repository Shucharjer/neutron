#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

int main() {

    // null_guard
    {
        {
            int val = 0;
            {
                null_gurad guard([&val] { ++val; });
            }
            require_or_return(val == 0, 1);
        }
    }

    // completion_guard
    {
        {
            int val = 0;
            {
                auto guard = make_exception_guard([&val]() noexcept { ++val; });
                guard.dismiss();
            }
            require_or_return(val == 0, 1);
        }

        {
            int val = 0;
            {
                auto guard = make_exception_guard([&val]() noexcept { ++val; });
            }
            require_or_return(val == 1, 1);
        }
    }

    return 0;
}
