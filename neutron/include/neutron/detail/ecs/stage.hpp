// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <compare>
#include <cstdint>
#include <type_traits>
#include "neutron/detail/metafn/definition.hpp"

namespace neutron {

enum class stage : uint8_t {
    pre_startup,
    startup,
    post_startup,
    first,
    pre_update,
    update,
    post_update,
    render,
    last,
    shutdown
};

constexpr std::strong_ordering operator<=>(stage lhs, stage rhs) noexcept {
    return static_cast<uint8_t>(lhs) <=> static_cast<uint8_t>(rhs);
}

template <stage Stage, typename... Tys>
struct staged_type_list {
    static constexpr auto stage = Stage;
    using type                  = type_list<Tys...>;
};

template <stage Stage, typename StagedTypeList>
struct is_staged_type_list : std::false_type {};
template <stage Stage, typename... Tys>
struct is_staged_type_list<Stage, staged_type_list<Stage, Tys...>> :
    std::true_type {};

template <stage Stage, typename TypeList>
struct _to_staged;
template <
    stage Stage, template <typename...> typename Template, typename... Tys>
struct _to_staged<Stage, Template<Tys...>> {
    using type = staged_type_list<Stage, Tys...>;
};
template <stage Stage, typename TypeList>
using _to_staged_t = typename _to_staged<Stage, TypeList>::type;

} // namespace neutron
