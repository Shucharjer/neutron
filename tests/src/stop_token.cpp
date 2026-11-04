#include <neutron/stop_token.hpp>

using namespace neutron;

namespace {

static_assert(stoppable_token<inplace_stop_token>);
static_assert(stoppable_token<stop_token>);
static_assert(unstoppable_token<never_stop_token>);
static_assert(!unstoppable_token<inplace_stop_token>);

bool test_never_stop_token() {
    constexpr never_stop_token token{};
    return !token.stop_possible() && !token.stop_requested();
}

bool test_inplace_stop_source() {
    inplace_stop_source source;
    auto token = source.get_token();
    if (!token.stop_possible() || token.stop_requested()) {
        return false;
    }
    if (!source.request_stop() || source.request_stop()) {
        return false; // only the first request succeeds
    }
    return source.stop_requested() && token.stop_requested();
}

bool test_inplace_stop_callback() {
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
    if (calls != 1) {
        return false;
    }

    // Registered after the request: invoked immediately.
    [[maybe_unused]] const callback_t after(source.get_token(), callback);
    return calls == 2;
}

bool test_stop_source() {
    stop_source source;
    auto token = source.get_token();
    source.request_stop();
    return token.stop_requested();
}

} // namespace

int main() {
    if (!test_never_stop_token()) {
        return 1;
    }
    if (!test_inplace_stop_source()) {
        return 2;
    }
    if (!test_inplace_stop_callback()) {
        return 3;
    }
    if (!test_stop_source()) {
        return 4;
    }
    return 0;
}
