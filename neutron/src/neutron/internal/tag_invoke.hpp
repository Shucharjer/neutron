#pragma once
#include <utility>

namespace neutron {

template <typename Tag, typename... Args>
concept tag_invocable = requires(Args&&... args) {
    tag_invoke(Tag{}, std::forward<Args>(args)...);
};

template <typename Tag, typename... Args>
concept nothrow_tag_invocable =
    tag_invocable<Tag, Args...> && requires(Args... args) {
        { tag_invoke(Tag{}, std::forward<Args>(args)...) } noexcept;
    };

} // namespace neutron
