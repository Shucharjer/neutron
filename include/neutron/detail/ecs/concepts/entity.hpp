// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <cstdint>
    #include <format>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT using entity_t     = std::uint64_t;
ATOM_EXPORT using generation_t = std::uint32_t;
ATOM_EXPORT using index_t      = std::uint32_t;

/**
 * @brief Future entity.
 * It mainly used in command_buffer.
 */
ATOM_EXPORT class future_entity_t {
public:
    constexpr explicit future_entity_t(index_t inframe_index) noexcept
        : identity_(inframe_index) {}

    ATOM_ATTRS(ATOM_NODISCARD) constexpr index_t get() const noexcept { return identity_; }

private:
    index_t identity_;
};

} // namespace neutron

namespace std {

template <typename CharT>
struct formatter<neutron::future_entity_t, CharT> {
    formatter<neutron::entity_t, CharT> underlying;
    constexpr auto parse(auto& ctx) { return underlying.parse(ctx); }
    template <typename FormatContext>
    constexpr auto format(
        const neutron::future_entity_t entity, FormatContext& ctx) const {
        return underlying.format(entity.get(), ctx);
    }
};

} // namespace std
