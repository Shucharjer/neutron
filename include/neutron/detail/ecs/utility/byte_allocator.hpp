#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <concepts>
    #include <memory>
#endif

namespace neutron::internal {

template <typename Alloc>
concept byte_allocator =
    std::same_as<typename std::allocator_traits<Alloc>::value_type, std::byte>;

} // namespace neutron::internal
