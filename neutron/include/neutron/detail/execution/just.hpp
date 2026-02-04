#pragma once
#include <utility>
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/make_sender.hpp"
#include "neutron/detail/execution/product_type.hpp"

namespace neutron::execution {

struct just_sender {
    using sender_concept = sender_t;

    template <typename... Args>
    just_sender(Args&&...) {}

    auto get_env() const noexcept { return empty_env{}; }
};

struct just_t {
    template <typename... Args>
    auto operator()(Args&&... args) const noexcept(
        (std::is_nothrow_constructible_v<std::decay_t<Args>, Args> && ...)) {
        return make_sender(*this, _product_type{ std::forward<Args>(args)... });
    }
};

inline constexpr just_t just;

} // namespace neutron::execution
