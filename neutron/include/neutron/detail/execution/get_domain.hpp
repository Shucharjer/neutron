#pragma once
#include <cstddef>

namespace neutron::execution {

template <typename Sndr>
constexpr auto _get_domain_early(const Sndr& sndr) noexcept;

template <typename Sndr, typename Env>
constexpr auto _get_domain_late(const Sndr& sndr, const Env& env) noexcept;

}