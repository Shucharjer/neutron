#pragma once

namespace neutron {

// [stoptoken], class stop_token
class stop_token;

// [stopsource], class stop_source
class stop_source;

// no-shared-stop-state indicator
struct nostopstate_t {
    explicit nostopstate_t() = default;
};
inline constexpr nostopstate_t nostopstate{};

// [stopcallback], class template stop_callback
template <class Callback>
class stop_callback;

// [stoptoken.never], class never_stop_token
class never_stop_token;

// [stoptoken.inplace], class inplace_stop_token
class inplace_stop_token;

// [stopsource.inplace], class inplace_stop_source
class inplace_stop_source;

// [stopcallback.inplace], class template inplace_stop_callback
template <class CallbackFn>
class inplace_stop_callback;

template <class T, class CallbackFn>
using stop_callback_for_t = T::template callback_type<CallbackFn>;

} // namespace neutron
