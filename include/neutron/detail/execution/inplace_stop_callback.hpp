#pragma once

namespace neutron {

template <typename CallbackFn>
class inplace_stop_callback;

template <typename T, typename CallbackFn>
using stop_callback_for_t = T::template callback_type<CallbackFn>;

} // namespace neutron
