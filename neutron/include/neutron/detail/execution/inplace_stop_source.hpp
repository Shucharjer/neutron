#pragma once
#include "neutron/detail/execution/inplace_stop_token.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron {

class inplace_stop_source {
public:
    ATOM_NODISCARD bool stop_requested() const noexcept;
    static constexpr bool stop_possible() noexcept;
    ATOM_NODISCARD auto get_token() const -> inplace_stop_token;

    bool request_stop();

private:
};

} // namespace neutron::execution
