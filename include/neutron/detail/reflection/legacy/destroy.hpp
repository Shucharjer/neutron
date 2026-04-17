// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include <memory>
#include <type_traits>

/*! @cond TURN_OFF_DOXYGEN */
namespace neutron::_refl_legacy {

template <typename Ty>
constexpr void
    wrapped_destroy(void* ptr) noexcept(std::is_nothrow_destructible_v<Ty>) {
    std::destroy_at(static_cast<Ty*>(ptr));
}

} // namespace neutron::_refl_legacy
/*! @endcond */
