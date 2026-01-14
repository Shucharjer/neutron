// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <cstdint>
#include <format>
#include <neutron/detail/macros.hpp>


namespace neutron {

using entity_t     = uint64_t;
using generation_t = uint32_t;
using index_t      = uint32_t;

/**
 * @brief Future entity.
 * It mainly used in command_buffer.
 */
class future_entity_t {
public:
    constexpr explicit future_entity_t(index_t inframe_index)
        : identity_(inframe_index) {}

    ATOM_NODISCARD constexpr index_t get() const noexcept { return identity_; }

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
    constexpr auto
        format(const neutron::future_entity_t entity, FormatContext& ctx) const {
        return underlying.format(entity.get(), ctx);
    }
};

} // namespace std
