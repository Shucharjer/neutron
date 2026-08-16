// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once

// some errors would be caused when compile simdjson.h with gcc
#if __has_include(<simdjson.h>)
    #include <cstddef>
    #include <string>
    #include <string_view>
    #include <type_traits>
    #include <utility>
    #include <simdjson.h>
    #include "neutron/detail/macros.hpp"
    #include "neutron/detail/reflection/refl.hpp"
    #include "neutron/detail/utility/get.hpp"

    #if SIMDJSON_SUPPORTS_CONCEPTS

/*! @cond TURN_OFF_DOXYGEN */
namespace neutron::internal {

/**
 * @brief Types that neutron reflection maps onto a JSON object.
 *
 * `reflectible` is satisfied by every aggregate, which is far wider than what
 * should be written as a JSON object: simdjson already knows how to read and
 * write strings, containers, maps, optionals and smart pointers, and those
 * mappings must win. Everything simdjson handles itself is therefore excluded
 * here so that only "plain structs" reach the reflection based bridge.
 *
 * Note that `simdjson::concepts::constructible_from_string_view` is *not*
 * excluded: C++20 parenthesized aggregate initialization makes most aggregates
 * whose first member is a string "constructible from a string_view", so
 * excluding it would send ordinary structs down simdjson's string path.
 */
template <typename Ty>
concept simdjson_object_like =
    ::neutron::reflectible<Ty> && ::std::is_class_v<Ty> &&
    !::simdjson::concepts::string_like<Ty> &&
    !::simdjson::concepts::container_but_not_string<Ty> &&
    !::simdjson::concepts::appendable_containers<Ty> &&
    !::simdjson::concepts::string_view_keyed_map<Ty> &&
    !::simdjson::concepts::optional_type<Ty> &&
    !::simdjson::concepts::smart_pointer<Ty> &&
    !::simdjson::concepts::is_pair<Ty> &&
    !::std::is_convertible_v<Ty, ::std::string_view>;

/**
 * @brief Read a single member of a reflectible aggregate.
 *
 * @param object The JSON object holding the aggregate.
 * @param name Name of the member, as reported by reflection.
 * @param member The member to read into.
 * @return An error code, `SUCCESS` on success.
 */
template <typename Member>
ATOM_NODISCARD inline auto deserialize_member(
    ::simdjson::ondemand::object& object, ::std::string_view name,
    Member& member) -> ::simdjson::error_code {
    auto field = object[name];

    if constexpr (::simdjson::concepts::optional_type<Member>) {
        // simdjson maps an explicit `null` onto an empty optional, but a key
        // that is not there at all is not an error for an optional member.
        if (field.error() == ::simdjson::NO_SUCH_FIELD) {
            member.reset();
            return ::simdjson::SUCCESS;
        }
    }

    if constexpr (::std::is_enum_v<Member>) {
        // simdjson only reads enums when static reflection is available, so
        // read the underlying integer instead.
        ::std::underlying_type_t<Member> value{};
        if (const auto error = field.get(value); error) {
            return error;
        }
        member = static_cast<Member>(value);
        return ::simdjson::SUCCESS;
    } else {
        return field.get(member);
    }
}

/**
 * @brief Read a reflectible aggregate from a document, value or object.
 *
 * @return An error code, `SUCCESS` on success. The first member that fails to
 * be read stops the walk and its error is returned.
 */
template <typename SimdjsonValue, simdjson_object_like Ty>
ATOM_NODISCARD inline auto deserialize_object(SimdjsonValue& value, Ty& object)
    -> ::simdjson::error_code {
    ::simdjson::ondemand::object json{};
    if constexpr (::std::is_same_v<
                      ::std::remove_cv_t<SimdjsonValue>,
                      ::simdjson::ondemand::object>) {
        json = value;
    } else {
        if (const auto error = value.get_object().get(json); error) {
            return error;
        }
    }

    [[maybe_unused]] constexpr auto names = ::neutron::member_names_of<Ty>();
    auto error                            = ::simdjson::SUCCESS;
    [&]<::std::size_t... Is>(::std::index_sequence<Is...>) {
        // Short-circuits on the first member that fails.
        (((error = deserialize_member(
               json, names[Is], ::neutron::get<Is>(object))) ==
          ::simdjson::SUCCESS) &&
         ...);
    }(::std::make_index_sequence<::neutron::member_count_of<Ty>()>());
    return error;
}

/**
 * @brief Write a single member of a reflectible aggregate.
 */
template <typename Member>
inline void serialize_member(
    ::simdjson::builder::string_builder& builder, const Member& member) {
    if constexpr (::std::is_enum_v<Member>) {
        // Mirrors deserialize_member(): enums travel as their underlying
        // integer.
        builder.append(static_cast<::std::underlying_type_t<Member>>(member));
    } else if constexpr (::std::is_same_v<Member, char>) {
        // string_builder::append(char) writes the raw character, which is not
        // a JSON value on its own.
        builder.escape_and_append_with_quotes(member);
    } else {
        builder.append(member);
    }
}

/**
 * @brief Types that can be read from `Source` through `Source::get`.
 */
template <typename Source, typename Ty>
concept simdjson_readable = requires(Source& source, Ty& object) {
    { source.get(object) } -> ::std::same_as<::simdjson::error_code>;
};

    /**
     * @brief Types that simdjson knows how to write as JSON.
     */
        #if SIMDJSON_STATIC_REFLECTION
template <typename Ty>
concept simdjson_writable =
    requires(::simdjson::builder::string_builder& builder, const Ty& object) {
        ::simdjson::builder::append(builder, object);
    };
        #else
template <typename Ty>
concept simdjson_writable =
    requires(::simdjson::builder::string_builder& builder, const Ty& object) {
        builder.append(object);
    };
        #endif

} // namespace neutron::internal
    /*! @endcond */

        #if !SIMDJSON_STATIC_REFLECTION

namespace simdjson {

/**
 * @brief Reads a neutron-reflectible aggregate as a JSON object.
 *
 * Not `noexcept`: reading a member may allocate, and simdjson computes the
 * exception specification of `value::get` from this one.
 */
template <typename Ty, typename SimdjsonValue>
requires ::neutron::internal::simdjson_object_like<Ty> &&
         (!concepts::constructible_from_string_view<Ty>)
auto tag_invoke(deserialize_tag, SimdjsonValue& value, Ty& object)
    -> error_code {
    return ::neutron::internal::deserialize_object(value, object);
}

/**
 * @brief Reads a neutron-reflectible aggregate as a JSON object.
 *
 * Same as above, but for aggregates that C++20 parenthesized aggregate
 * initialization makes constructible from a `std::string_view`. The constraints
 * are deliberately written so that this overload subsumes -- and therefore
 * wins against -- simdjson's own string-like overload, which would otherwise
 * be ambiguous with it.
 */
template <concepts::constructible_from_string_view Ty, typename SimdjsonValue>
requires ::neutron::internal::simdjson_object_like<Ty>
auto tag_invoke(deserialize_tag, SimdjsonValue& value, Ty& object)
    -> error_code {
    return ::neutron::internal::deserialize_object(value, object);
}

/**
 * @brief Writes a neutron-reflectible aggregate as a JSON object.
 *
 * Members are written in declaration order.
 */
template <::neutron::internal::simdjson_object_like Ty>
void tag_invoke(
    serialize_tag, builder::string_builder& builder, const Ty& object) {
    [[maybe_unused]] constexpr auto names = ::neutron::member_names_of<Ty>();
    builder.start_object();
    [&]<::std::size_t... Is>(::std::index_sequence<Is...>) {
        (
            [&] {
                if constexpr (Is != 0) {
                    builder.append_comma();
                }
                builder.escape_and_append_with_quotes(names[Is]);
                builder.append_colon();
                ::neutron::internal::serialize_member(
                    builder, ::neutron::get<Is>(object));
            }(),
            ...);
    }(::std::make_index_sequence<::neutron::member_count_of<Ty>()>());
    builder.end_object();
}

} // namespace simdjson

        #endif // !SIMDJSON_STATIC_REFLECTION

namespace neutron {

/**
 * @brief Reads @p object out of a simdjson document, value or object.
 *
 * @param source A `simdjson::ondemand` document, value or object (or a
 * `simdjson_result` of one of them).
 * @param object The object to read into.
 * @return An error code, `simdjson::SUCCESS` on success.
 */
template <typename Ty, typename Source>
requires internal::simdjson_readable<::std::remove_reference_t<Source>, Ty>
ATOM_NODISCARD inline auto deserialize(Source&& source, Ty& object)
    -> simdjson::error_code {
    return source.get(object);
}

/**
 * @brief Reads a `Ty` out of a simdjson document, value or object.
 *
 * @param source A `simdjson::ondemand` document, value or object (or a
 * `simdjson_result` of one of them).
 * @return The parsed object, or the error that stopped the parse.
 */
template <typename Ty, typename Source>
requires internal::simdjson_readable<::std::remove_reference_t<Source>, Ty> &&
         ::std::is_default_constructible_v<Ty>
ATOM_NODISCARD inline auto deserialize(Source&& source)
    -> simdjson::simdjson_result<Ty> {
    Ty object{};
    if (const auto error = source.get(object); error) {
        return error;
    }
    return ::std::move(object);
}

/**
 * @brief Writes @p object into @p builder.
 */
template <internal::simdjson_writable Ty>
inline void
    serialize(simdjson::builder::string_builder& builder, const Ty& object) {
        #if SIMDJSON_STATIC_REFLECTION
    simdjson::builder::append(builder, object);
        #else
    builder.append(object);
        #endif
}

/**
 * @brief Writes @p object into @p out as JSON.
 *
 * @return An error code, `simdjson::SUCCESS` on success.
 */
template <internal::simdjson_writable Ty>
ATOM_NODISCARD inline auto serialize(::std::string& out, const Ty& object)
    -> simdjson::error_code {
    return simdjson::to_json(object, out);
}

/**
 * @brief Writes @p object as JSON.
 *
 * @return The JSON text, or the error that stopped the write.
 */
template <internal::simdjson_writable Ty>
ATOM_NODISCARD inline auto serialize(const Ty& object)
    -> simdjson::simdjson_result<::std::string> {
    return simdjson::to_json(object);
}

} // namespace neutron

    #endif // SIMDJSON_SUPPORTS_CONCEPTS

#endif
