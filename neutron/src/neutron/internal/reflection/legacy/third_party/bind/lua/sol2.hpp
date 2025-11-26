#pragma once

#if __has_include(<lua.hpp>) && __has_include(<sol/sol.hpp>)
    #include <type_traits>
    #include <sol/sol.hpp>

namespace neutron::internal {
template <::neutron::concepts::reflectible Ty>
nlohmann::json to_json(const Ty& obj) {
    nlohmann::json json;
    ::to_json(json, obj);
    return json;
}

template <::neutron::concepts::reflectible Ty>
void from_json(Ty& obj, const nlohmann::json& json) {
    ::from_json(json, obj);
}
} // namespace neutron::internal

namespace neutron {

template <::neutron::concepts::pure Ty>
inline sol::usertype<Ty> bind_to_lua(sol::state& lua) noexcept {
    constexpr auto name = ::neutron::name_of<Ty>();
    auto usertype       = lua.new_usertype<Ty>();

    if constexpr (std::is_default_constructible_v<Ty>) {
        usertype["new"] = sol::constructors<Ty()>();
    }

    constexpr auto reflected = ::neutron::reflected<Ty>();
    if constexpr (::neutron::concepts::reflectible<Ty>) {
        if constexpr (::neutron::concepts::aggregate<Ty>) {
            const auto& fields = reflected.fields();
            [&]<size_t... Is>(std::index_sequence<Is...>) {
                ((usertype[std::get<Is>(fields).name()] =
                      std::get<Is>(fields).pointer()),
                 ...);
            }(std::make_index_sequence<std::tuple_size_v<decltype(fields)>>());
        } else if constexpr (::neutron::concepts::has_field_traits<Ty>) {
            constexpr auto& fields = reflected.fields();
            [&]<size_t... Is>(std::index_sequence<Is...>) {
                ((usertype[std::get<Is>(fields).name()] =
                      std::get<Is>(fields).pointer()),
                 ...);
            }(std::make_index_sequence<std::tuple_size_v<decltype(fields)>>());
        }

    #if __has_include(<nlohmann/json.hpp>)
        usertype["to_json"]   = &internal::to_json<Ty>;
        usertype["from_json"] = &internal::from_json<Ty>;
    #endif
    }

    if constexpr (::neutron::concepts::has_function_traits<Ty>) {
        const auto& funcs = reflected.functions();
        [&]<size_t... Is>(std::index_sequence<Is...>) {
            ((usertype[std::get<Is>(funcs).name()] =
                  std::get<Is>(funcs).pointer()),
             ...);
        }(std::make_index_sequence<std::tuple_size_v<decltype(funcs)>>());
    }
}

} // namespace neutron

#endif
