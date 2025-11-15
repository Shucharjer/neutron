#pragma once
#include <utility>

namespace neutron {

template <typename Tag, typename... Args>
using tag_invoke_result_t =
    decltype(tag_invoke(std::declval<Tag>(), std::declval<Args>()...));

template <typename Tag, typename... Args>
concept tag_invocable = requires(Tag tag, Args&&... args) {
    tag_invoke(tag, std::forward<Args>(args)...);
};

struct tag_invoke_t {
    template <typename Tag, typename... Args>
    constexpr auto operator()([[maybe_unused]] Tag tag, Args&&... args) const
        noexcept(noexcept(tag_invoke(tag, std::forward<Args>(args)...))) {
        return tag_invoke(tag, std::forward<Args>(args)...);
    }
};

} // namespace neutron
