// IWYU pragma: private, include <neutron/concepts.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <utility>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename Tag, typename... Args>
concept tag_invocable = requires(Args&&... args) {
    tag_invoke(Tag{}, std::forward<Args>(args)...);
};

ATOM_EXPORT template <typename Tag, typename... Args>
concept nothrow_tag_invocable =
    tag_invocable<Tag, Args...> && requires(Args... args) {
        { tag_invoke(Tag{}, std::forward<Args>(args)...) } noexcept;
    };

} // namespace neutron
