#pragma once

namespace neutron::execution {

struct _on_stop_request {
    inplace_stop_source& stop_src;
    void operator()() noexcept { stop_src.request_stop(); }
};

} // namespace neutron::execution
