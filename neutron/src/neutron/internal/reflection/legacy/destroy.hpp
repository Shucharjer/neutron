#pragma once
#include <memory>
#include <type_traits>


namespace neutron::_reflection {

template <typename Ty>
constexpr void
    wrapped_destroy(void* ptr) noexcept(std::is_nothrow_destructible_v<Ty>) {
    std::destroy_at(static_cast<Ty*>(ptr));
}

} // namespace neutron::_reflection
