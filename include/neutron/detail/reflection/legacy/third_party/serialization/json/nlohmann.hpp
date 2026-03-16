#pragma once

#if __has_include(<nlohmann/json.hpp>)
    #include <nlohmann/json.hpp>

template <::neutron::_reflection::reflectible Ty>
inline void to_json(nlohmann::json& json, const Ty& obj) {
    constexpr auto names = neutron::member_names_of<Ty>();
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        ((json[names[Is]] = ::neutron::get<Is>(obj)), ...);
    }(std::make_index_sequence<neutron::member_count_v<Ty>>());
}

template <::neutron::_reflection::reflectible Ty>
inline void from_json(const nlohmann::json& json, Ty& obj) {
    constexpr auto names = neutron::member_names_of<Ty>();
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        ((json.at(names[Is]).get_to(::neutron::get<Is>(obj))), ...);
    }(std::make_index_sequence<neutron::member_count_v<Ty>>());
}

    #define NLOHMANN_JSON_SUPPORT                                              \
        template <::neutron::_reflection::reflectible Ty>                      \
        inline void to_json(nlohmann::json& json, const Ty& obj) {             \
            ::to_json(json, obj);                                              \
        }                                                                      \
        template <::neutron::_reflection::reflectible Ty>                      \
        inline void from_json(const nlohmann::json& json, Ty& obj) {           \
            ::from_json(json, obj);                                            \
        }                                                                      \
        //

template <>
struct neutron::serialization<nlohmann::json> {
    template <typename Ty>
    auto operator()(const Ty& obj, nlohmann::json& json) const {
        if constexpr (::neutron::_reflection::reflectible<Ty>) {
            ::to_json(json, obj);
        } else {
            json = obj;
        }
    }
};

template <>
struct neutron::deserialization<nlohmann::json> {
    template <typename Ty>
    auto operator()(Ty& obj, const nlohmann::json& json) const {
        if constexpr (::neutron::_reflection::reflectible<Ty>) {
            ::from_json(json, obj);
        } else {
            obj = json;
        }
    }
};

#endif
