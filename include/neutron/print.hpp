#pragma once

#if defined(ATOM_USES_MODULES) && !defined(ATOM_BUILDING_MODULE)
    #include "neutron/macros.hpp"
#else

    #include "neutron/detail/utility/macros.hpp"

    #ifndef ATOM_BUILDING_MODULE
        #include <ostream>
        #include <version>
    #endif

    #if defined(__cpp_lib_print) && __cpp_lib_print >= 202207L

        #ifndef ATOM_BUILDING_MODULE
            #include <print>
        #endif

namespace neutron {

ATOM_EXPORT using std::print;
ATOM_EXPORT using std::println;

} // namespace neutron

    #else

        #ifndef ATOM_BUILDING_MODULE
            #include <format>
            #include <iostream>
        #endif

namespace neutron {

ATOM_EXPORT template <typename... Args>
inline void print(std::format_string<Args...> fmt, Args&&... args) {
    std::cout << std::format(fmt, std::forward<Args>(args)...);
}

ATOM_EXPORT template <typename... Args>
inline void
    print(std::ostream& os, std::format_string<Args...> fmt, Args&&... args) {
    os << std::format(fmt, std::forward<Args>(args)...);
}

ATOM_EXPORT template <typename... Args>
inline void println(std::format_string<Args...> fmt, Args&&... args) {
    std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
}

ATOM_EXPORT template <typename... Args>
inline void
    println(std::ostream& os, std::format_string<Args...> fmt, Args&&... args) {
    os << std::format(fmt, std::forward<Args>(args)...) << '\n';
}

} // namespace neutron

    #endif

#endif
