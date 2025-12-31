// IWYU pragma: private, include "neutron/memory.hpp"
#pragma once
#include <memory>

namespace neutron {

template <typename Alloc, typename Ty>
struct rebind_alloc {
    using type =
        typename std::allocator_traits<Alloc>::template rebind_alloc<Ty>;
};

template <typename Alloc, typename Ty>
using rebind_alloc_t = typename rebind_alloc<Alloc, Ty>::type;

} // namespace neutron
