// private, include <neutron/ecs.hpp>
#pragma once
#include <array>
#include <type_traits>
#include <neutron/concepts.hpp>
#include <neutron/detail/ecs/fwd.hpp>
#include "neutron/detail/ecs/archetype.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron {

template <component... Components>
class slice {

    static_assert(!(std::is_empty_v<Components> || ...));

    static_assert(
        !(std::is_reference_v<Components> || ...),
        "slice do not need to retain access information; using primitive types reduces binary bloat.");

public:
    using value_type      = std::tuple<Components...>;
    using reference       = value_type;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    template <typename Alloc>
    slice(archetype<Alloc>& archetype) noexcept
        : size_(&archetype.size()),
          entities_(&archetype._entities()),
          bufs_([&archetype]<size_t... Is>(std::index_sequence<Is...>) {
              const auto buffers =
                  archetype.template view_buffers<Components...>();
              return std::array<_buffer_ptr*, sizeof...(Components)>{
                  std::get<Is>(buffers)...
              };
          }(std::index_sequence_for<Components...>())) {}

    ATOM_NODISCARD constexpr size_type size() const noexcept { return *size_; }

    ATOM_NODISCARD constexpr const entity_t* entities() const noexcept {
        return *entities_;
    }

    ATOM_NODISCARD constexpr auto buffers() const noexcept
        -> const std::array<_buffer_ptr*, sizeof...(Components)>& {
        return bufs_;
    }

private:
    const size_t* size_;
    const entity_t* const* entities_;
    std::array<_buffer_ptr*, sizeof...(Components)> bufs_;
};

template <component... Components, std_simple_allocator Alloc>
ATOM_NODISCARD auto slice_of(archetype<Alloc>& archetype) noexcept {
    using component_list = type_list<std::remove_cvref_t<Components>...>;
    return type_list_rebind_t<slice, component_list>{ archetype };
}

} // namespace neutron
