#pragma once

#if defined(NDEBUG)
    #define NEUTRON_ASSERT(expr)
#else
    #include <cassert>
    #define NEUTRON_ASSERT(expr) assert(expr)
#endif
