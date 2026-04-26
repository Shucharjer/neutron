#pragma once
#include <memory>
#include "neutron/detail/macros.hpp"
#include "neutron/detail/stop_token/fwd.hpp"
#include "neutron/detail/stop_token/stop_token.hpp"

namespace neutron {

class stop_source {
public:
    // [stopsource.cons], constructors, copy, and assignment
    stop_source();
    explicit stop_source(nostopstate_t) noexcept {}

    // [stopsource.mem], member functions
    void swap(stop_source&) noexcept;

    ATOM_NODISCARD stop_token get_token() const noexcept;
    ATOM_NODISCARD bool stop_possible() const noexcept;
    ATOM_NODISCARD bool stop_requested() const noexcept;
    bool request_stop() noexcept;

    bool operator==(const stop_source& rhs) const noexcept = default;

private:
    using _unspecified = void;
    std::shared_ptr<_unspecified> stop_state_; // exposition only
};

} // namespace neutron
