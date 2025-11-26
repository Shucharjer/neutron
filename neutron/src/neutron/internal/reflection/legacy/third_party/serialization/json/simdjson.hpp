#pragma once

// some errors would be caused when compile simdjson.h with gcc
#if !defined(__GNUC__) && __has_include(<simdjson.h>)
    #include <string>
    #include <string_view>
    #include <simdjson.h>

/*! @cond TURN_OFF_DOXYGEN */
namespace neutron::internal {
template <size_t Index, typename Ty, typename Val>
inline auto tag_invoke_impl(Ty&& obj, Val& val) {
    using type = std::remove_cvref_t<Val>;
    if constexpr (std::is_same_v<type, uint64_t>) {
        return obj.get_uint64(val);
    } else if constexpr (std::is_same_v<type, int64_t>) {
        return obj.get_int64(val);
    } else if constexpr (std::is_same_v<type, double>) {
        return obj.get_double(val);
    } else if constexpr (std::is_same_v<type, bool>) {
        return obj.get_bool(val);
    } else if constexpr (
        std::is_same_v<type, std::string_view> ||
        std::is_same_v<type, std::string>) {
        return obj.get_string(val);
    } else if constexpr (std::is_same_v<type, simdjson::ondemand::object>) {
        return obj.get_object(val);
    } else if constexpr (std::is_same_v<type, simdjson::ondemand::array>) {
        return obj.get_array(val);
    } else {
        return obj.get(val);
    }
}
} // namespace neutron::internal
/*! @endcond */

namespace simdjson {
/**
 * @brief Deserialization support for simdjson
 *
 */
template <typename simdjson_value, ::neutron::concepts::reflectible Ty>
auto tag_invoke(
    simdjson::deserialize_tag, simdjson_value& val,
    Ty& object) noexcept // it would return error code
{
    ondemand::object obj;
    auto error = val.get_object().get(obj);
    if (error) [[unlikely]] {
        return error;
    }

    constexpr auto names = ::neutron::member_names_of<Ty>();
    [&]<size_t... Is>(::std::index_sequence<Is...>) {
        (::internal::tag_invoke_impl<Is>(
             obj[names[Is]], ::neutron::get<Is>(object)),
         ...);
    }(::std::make_index_sequence<neutron::member_count_v<Ty>>());

    return simdjson::SUCCESS;
}
} // namespace simdjson

template <>
struct ::neutron::deserialization<simdjson::fallback::ondemand::document> {
    template <typename Ty>
    auto
        operator()(Ty& obj, simdjson::fallback::ondemand::document& doc) const {
        ::simdjson::deserialize(doc, obj);
    }
};

#endif
