#pragma once
// IWYU pragma: begin_exports
#include <ranges>
#include "neutron/detail/ranges/adaptor_closure.hpp"
#include "neutron/detail/ranges/closure.hpp"
#include "neutron/detail/ranges/concepts.hpp"
#include "neutron/detail/ranges/to.hpp"
#include "neutron/detail/ranges/views/elements_view.hpp"
// IWYU pragma: end_exports

namespace neutron {

namespace ranges::views {};

namespace views = ranges::views;

} // namespace neutron
