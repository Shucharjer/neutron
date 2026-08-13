#pragma once

#if defined(NDEBUG)
    #define NEUTRON_ASSERT(expr)
#else
    #include <cassert>
    #define NUETRON_ASSERT(expr) assert(expr)
#endif
