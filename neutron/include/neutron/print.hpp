#pragma once
#include <version>

#if defined(__cpp_lib_print) && __cpp_lib_print >= 202207L

    #include <print>

namespace neutron {

using std::print;
using std::println;

} // namespace neutron

#else

    #include <format>
    #include <iostream>

namespace neutron {

template <typename... Args>
inline void print(std::format_string<Args...> fmt, Args&&... args) {
    std::cout << std::format(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void println(std::format_string<Args...> fmt, Args&&... args) {
    std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
}

} // namespace neutron

#endif
