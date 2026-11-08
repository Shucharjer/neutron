#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <neutron/print.hpp>
#include <neutron/string.hpp>

template <typename T>
struct throw_allocator {
    using value_type      = T;
    using size_type       = std::size_t;
    using pointer         = T*;
    using difference_type = std::ptrdiff_t;
    pointer allocate(std::size_t n) {
        throw std::logic_error("should not allocate");
    }
    void deallocate(pointer, std::size_t) const noexcept {}
};

int main() {

    {
        using alloc_t  = throw_allocator<char>;
        using traits_t = std::char_traits<char>;
        using sstring =
            neutron::basic_short_string<16, char, traits_t, alloc_t>;
        sstring ss = "short string";

        neutron::println("{}", ss);
        try {
            sstring ls = "this is a long string exceeds 16 bytes";
        } catch (...) {
            neutron::println("through as expect");
        }

        neutron::short_string<16> ls = "stores a long string exceeds 16 bytes";
        neutron::println("{}", ls);
    }

    return 0;
}
