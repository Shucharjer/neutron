#pragma once
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/senders/default_domain.hpp"

namespace neutron::execution {

template <typename Domain, typename Tag, sender Sndr, typename... Args>
constexpr decltype(auto)
    apply_sender(Domain dom, Tag, Sndr&& sndr, Args&&... args) noexcept([] {
        if constexpr (requires {
                          dom.apply_sender(
                              Tag{}, std::forward<Sndr>(sndr),
                              std::forward<Args>(args)...);
                      }) {
            return noexcept(dom.apply_sender(
                Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...));
        } else {
            return noexcept(default_domain{}.apply_sender(
                Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...));
        }
    }()) {
    if constexpr (requires {
                      dom.apply_sender(
                          Tag{}, std::forward<Sndr>(sndr),
                          std::forward<Args>(args)...);
                  }) {
        return dom.apply_sender(
            Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    } else {
        return default_domain{}.apply_sender(
            Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...);
    }
}

} // namespace neutron::execution
