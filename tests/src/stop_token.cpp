#include <neutron/stop_token.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

// Token concepts
static_assert(stoppable_token<inplace_stop_token>);
static_assert(stoppable_token<stop_token>);
static_assert(unstoppable_token<never_stop_token>);
static_assert(!unstoppable_token<inplace_stop_token>);

void test_never_stop_token_never_stops() {
    constexpr never_stop_token token{};

    require_false(token.stop_possible());
    require_false(token.stop_requested());
}

void test_inplace_token_starts_unrequested() {
    inplace_stop_source source;

    auto token = source.get_token();

    require(token.stop_possible());
    require_false(token.stop_requested());
}

void test_only_the_first_stop_request_succeeds() {
    inplace_stop_source source;

    require(source.request_stop());
    require_false(source.request_stop());
}

void test_inplace_stop_request_reaches_the_token() {
    inplace_stop_source source;
    auto token = source.get_token();

    source.request_stop();

    require(source.stop_requested());
    require(token.stop_requested());
}

void test_inplace_callback_runs_on_stop_request() {
    inplace_stop_source source;
    int calls     = 0;
    auto callback = [&calls]() noexcept {
        ++calls;
    };
    using callback_t = inplace_stop_callback<decltype(callback)>;

    {
        // Destroyed before the request, so never invoked.
        [[maybe_unused]] const callback_t gone(source.get_token(), callback);
    }
    [[maybe_unused]] const callback_t before(source.get_token(), callback);
    source.request_stop();

    require(calls == 1);
}

void test_inplace_callback_registered_after_stop_runs_immediately() {
    inplace_stop_source source;
    int calls     = 0;
    auto callback = [&calls]() noexcept {
        ++calls;
    };
    using callback_t = inplace_stop_callback<decltype(callback)>;
    source.request_stop();

    [[maybe_unused]] const callback_t after(source.get_token(), callback);

    require(calls == 1);
}

void test_stop_source_request_reaches_the_token() {
    stop_source source;
    auto token = source.get_token();

    source.request_stop();

    require(token.stop_requested());
}

} // namespace

int main() {
    test_never_stop_token_never_stops();
    test_inplace_token_starts_unrequested();
    test_only_the_first_stop_request_succeeds();
    test_inplace_stop_request_reaches_the_token();
    test_inplace_callback_runs_on_stop_request();
    test_inplace_callback_registered_after_stop_runs_immediately();
    test_stop_source_request_reaches_the_token();
    return neutron_test::result();
}
