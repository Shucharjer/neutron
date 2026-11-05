#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

void test_null_guard_never_runs_its_callback() {
    int calls = 0;

    {
        [[maybe_unused]] null_gurad guard([&calls] {
            ++calls;
        });
    }

    require(calls == 0);
}

void test_dismissed_exception_guard_does_not_run() {
    int calls = 0;

    {
        auto guard = make_exception_guard([&calls]() noexcept {
            ++calls;
        });
        guard.dismiss();
    }

    require(calls == 0);
}

void test_exception_guard_runs_when_not_dismissed() {
    int calls = 0;

    {
        auto guard = make_exception_guard([&calls]() noexcept {
            ++calls;
        });
    }

    require(calls == 1);
}

} // namespace

int main() {
    test_null_guard_never_runs_its_callback();
    test_dismissed_exception_guard_does_not_run();
    test_exception_guard_runs_when_not_dismissed();
    return neutron_test::result();
}
