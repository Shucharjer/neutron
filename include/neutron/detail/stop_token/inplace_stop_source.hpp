#pragma once
#include "neutron/detail/macros.hpp"
#include "neutron/detail/stop_token/fwd.hpp"
#include "neutron/detail/stop_token/inplace_stop_token.hpp"

namespace neutron {

class inplace_stop_source {
public:
    ATOM_NODISCARD bool stop_requested() const noexcept;
    static constexpr bool stop_possible() noexcept;
    ATOM_NODISCARD auto get_token() const -> inplace_stop_token;

    bool request_stop();

private:
};

struct _on_stop_request {
    inplace_stop_source& stop_src; // exposition only
    void operator()() noexcept { stop_src.request_stop(); }
};

} // namespace neutron
