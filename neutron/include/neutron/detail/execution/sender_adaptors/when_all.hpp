#pragma once
#include <concepts>
#include <type_traits>
#include "neutron/detail/execution/default_domain.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/get_domain.hpp"
#include "neutron/detail/execution/make_sender.hpp"

namespace neutron::execution {

inline constexpr struct when_all_t {
    template <sender... Senders>
    requires(sizeof...(Senders) != 0)
    constexpr auto operator()(Senders&&... senders) const {
        using common_t =
            std::common_type_t<decltype(_get_domain_early(senders))...>;
        return transform_sender(
            common_t{},
            make_sender(*this, {}, std::forward<Senders>(senders)...));
    }

} when_all{};

template <>
struct _impls_for<when_all_t> : _default_impls {
    static constexpr struct _get_attrs_impl {
        constexpr auto operator()(auto&&,auto&&...child) noexcept{
            // TODO
        }
    } get_attrs{};
}   ;

} // namespace neutron::execution
