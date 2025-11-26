#pragma once
#include "../../macros.hpp"

#if HAS_REFLECTION

    #include <meta>
    #include <vector>

namespace neutron {

namespace _meta {

template <auto... Vals>
struct _replicator_type {
    template <typename F>
    constexpr void operator>>(F body) const {
        (body.template operator()<Vals>(), ...);
    }
};

template <auto... Vals>
static constexpr _replicator_type<Vals...> _replicator = {};

} // namespace _meta

/*
usage: [:expand(...):] >> [&]<auto>{};
```
struct (lambda) {
    template <auto>
    constexpr auto operator()() {...}
};
```
*/
template <std::ranges::range Rng>
static consteval auto expand(Rng range) {
    std::vector<std::meta::info> args;
    for (auto elem : range) {
        args.push_back(std::meta::reflect_constant(elem));
    }
    return std::meta::substitute(^^_meta::_replicator, args);
}

consteval auto constructors_of(
    std::meta::info info, std::meta::access_context ctx) noexcept
    -> std::meta::info {
    using namespace std::meta;
    return std::define_static_array(
        members_of(info, ctx) | std::views::filter([](std::meta::info info) {
            return is_constructor(info) && !is_deleted(info);
        }));
}

consteval auto
    destructor_of(std::meta::info info, std::meta::access_context ctx) noexcept
    -> std::meta::info {
    using namespace std::meta;
    auto dtor = members_of(info, ctx) | std::views::filter([](auto info) {
                    return is_destructor(info);
                });
    return *std::begin(dtor);
}

consteval auto operator_functions_of(
    std::meta::info info, std::meta::access_context ctx) noexcept {
    using namespace std::meta;
    return std::define_static_array(
        members_of(info, ctx) | std::views::filter([](auto info) {
            return is_operator_function(info);
        }));
}

consteval auto nonstatic_functions_of(
    std::meta::info info, std::meta::access_context ctx) noexcept {
    using namespace std::meta;
    return std::define_static_array(
        members_of(info, ctx) | std::views::filter([](std::meta::info info) {
            return !is_static_member(info) && is_function(info);
        }));
}

consteval auto static_functions_of(
    std::meta::info info, std::meta::access_context ctx) noexcept {
    using namespace std::meta;
    return std::define_static_array(
        members_of(info, ctx) | std::views::filter([](std::meta::info info) {
            return is_static_member(info) && is_function(info);
        }));
}

consteval auto ordinary_nonstatic_functions_of(
    std::meta::info info, std::meta::access_context ctx) noexcept {
    using namespace std::meta;
    return std::define_static_array(
        nonstatic_functions_of(info, ctx) |
        std::views::filter([](std::meta::info info) {
            return !is_operator_function(info) && !is_constructor(info) &&
                   !is_destructor(info);
        }));
}

} // namespace neutron

#endif
