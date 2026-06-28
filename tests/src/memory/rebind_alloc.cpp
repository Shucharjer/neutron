#include <concepts>
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <neutron/memory.hpp>

using namespace neutron;

int main() {

    using alloc_t     = std::allocator<std::byte>;
    using pmr_alloc_t = std::pmr::polymorphic_allocator<std::byte>;

    static_assert(std::same_as<
                  rebind_alloc_t<alloc_t, std::uint64_t>,
                  std::allocator<std::uint64_t>>);

    static_assert(std::same_as<
                  rebind_alloc_t<pmr_alloc_t, std::uint64_t>,
                  std::pmr::polymorphic_allocator<std::uint64_t>>);

    return 0;
}
