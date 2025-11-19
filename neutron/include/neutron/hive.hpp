#pragma once
#include <memory>
#include <version>
#include "../src/neutron/internal/allocator.hpp"


#if defined(__cpp_lib_hive) && __cpp_lib_hive >= 202602L

namespace neutron {

template <typename Ty, _std_simple_allocator Alloc = std::allocator<Ty>>
using hive = std::hive<Ty, Alloc>;

}

#else

    #include "../src/neutron/internal/plf_hive.hpp"

namespace neutron {

template <typename Ty, _std_simple_allocator Alloc = std::allocator<Ty>>
using hive = plf::hive<Ty, Alloc>;

}

#endif
