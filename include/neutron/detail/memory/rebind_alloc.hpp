// IWYU pragma: private, include <neutron/memory.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <memory>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename Alloc, typename Ty>
struct rebind_alloc {
    using type =
        typename std::allocator_traits<Alloc>::template rebind_alloc<Ty>;
};

ATOM_EXPORT template <typename Alloc, typename Ty>
using rebind_alloc_t = typename rebind_alloc<Alloc, Ty>::type;

} // namespace neutron
