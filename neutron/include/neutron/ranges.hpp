#pragma once
#include <ranges> // IWYU pragma: export
#include "../src/neutron/internal/ranges/adaptor_closure.hpp" // IWYU pragma: export
#include "../src/neutron/internal/ranges/closure.hpp"  // IWYU pragma: export
#include "../src/neutron/internal/ranges/concepts.hpp" // IWYU pragma: export
#include "../src/neutron/internal/ranges/to.hpp"       // IWYU pragma: export
#include "../src/neutron/internal/ranges/views/elements_view.hpp" // IWYU pragma: export

namespace neutron {

namespace ranges::views {};

namespace views = ranges::views;

} // namespace neutron
