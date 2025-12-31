#pragma once
#include <string_view>
#include <tuple>
#include "neutron/tstring.hpp"
#include "./concepts.hpp"
#include "./destroy.hpp"
#include "./member_traits.hpp"
#include "./name.hpp"
#include "./type_hash.hpp"
#include "./type_traits.hpp"

namespace neutron {

/*! @cond TURN_OFF_DOXYGEN */
namespace _reflection {
constexpr static inline std::string_view empty_string;
constexpr static inline std::tuple<> empty_tuple{};
} // namespace _reflection
/*! @endcond */

/**
 * @brief Recording basic information about a type.
 *
 */
struct basic_reflected {
public:
    constexpr basic_reflected(const basic_reflected&) noexcept = default;
    constexpr basic_reflected&
        operator=(const basic_reflected&) noexcept = default;

    constexpr basic_reflected(basic_reflected&& obj) noexcept
        : name_(obj.name_), hash_(obj.hash_), description_(obj.description_) {}
    constexpr basic_reflected& operator=(basic_reflected&& obj) noexcept {
        name_        = obj.name_;
        hash_        = obj.hash_;
        description_ = obj.description_;
        destroy_     = obj.destroy_;
        return *this;
    }

    constexpr ~basic_reflected() noexcept = default;

    /**
     * @brief Get the name of this reflected type.
     *
     * @return std::string_view The name.
     */
    [[nodiscard]] constexpr auto name() const noexcept -> std::string_view {
        return name_;
    }

    /**
     * @brief Get the hash value of this reflected type.
     *
     * @return std::size_t
     */
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t {
        return hash_;
    }

    [[nodiscard]] constexpr auto traits() const noexcept -> traits_bits {
        return description_;
    }

    constexpr void destroy(void* ptr) const { destroy_(ptr); }

protected:
    constexpr explicit basic_reflected(
        std::string_view name, const std::size_t hash,
        const traits_bits description,
        void (*destroy)(void*) = nullptr) noexcept
        : name_(name), hash_(hash), description_(description),
          destroy_(destroy) {}

private:
    std::string_view name_;
    std::size_t hash_;
    traits_bits description_;
    void (*destroy_)(void*) = nullptr;
};

/**
 * @brief A type stores reflected information.
 *
 * @tparam Ty The reflected type.
 */
template <value Ty>
struct reflected final : public basic_reflected {
    constexpr reflected() noexcept
        : basic_reflected(
              name_of<Ty>(), hash_of<Ty>(), traits_of<Ty>(),
              &_reflection::wrapped_destroy<Ty>) {}

    reflected(const reflected&) noexcept            = default;
    reflected(reflected&&) noexcept                 = default;
    reflected& operator=(const reflected&) noexcept = default;
    reflected& operator=(reflected&&) noexcept      = default;
    constexpr ~reflected() noexcept                 = default;

private:
    static const auto& _traits()
    requires _reflection::default_reflectible_aggregate<Ty>
    {
        constexpr auto names = member_names_of<Ty>();
        const auto& offsets  = offsets_of<Ty>();
        static const auto tuple =
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::make_tuple(
                    field_traits<std::tuple_element_t<
                        Is, std::remove_cvref_t<decltype(offsets)>>>{
                        names[Is], std::get<Is>(offsets) }...);
            }(std::make_index_sequence<member_count_of<Ty>()>());
        return tuple;
    }

public:
    /**
     * @brief Fields exploded to outside in the type.
     *
     * You could get the constexpr val when Ty isn't a aggregate.
     * @return constexpr const auto& A tuple contains function traits.
     */
    constexpr const auto& fields() const noexcept {
        if constexpr (_reflection::default_reflectible_aggregate<Ty>) {
            // static, but not constexpr.
            return reflected::_traits();
        } else if constexpr (_reflection::has_field_traits<Ty>) {
            return Ty::field_traits();
        } else {
            return _reflection::empty_tuple;
        }
    }

    /**
     * @brief Functions exploded to outside in the type.
     *
     * @return constexpr const auto& A tuple contains function traits.
     */
    constexpr const auto& functions() const noexcept {
        if constexpr (_reflection::has_function_traits<Ty>) {
            return Ty::function_traits();
        } else {
            return _reflection::empty_tuple;
        }
    }
};

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {
template <::neutron::tstring Name, std::size_t Index, typename Tuple>
constexpr void find_traits(const Tuple& tuple, std::size_t& result) {
    if (std::get<Index>(tuple).name() == Name.val) {
        result = Index;
    }
}

template <tstring Name, typename Tuple>
consteval std::size_t index_of(const Tuple& tuple) noexcept {
    constexpr auto index_sequence =
        std::make_index_sequence<std::tuple_size_v<Tuple>>();
    std::size_t result = std::tuple_size_v<Tuple>;
    []<std::size_t... Is>(
        const Tuple& tuple, std::size_t& result, std::index_sequence<Is...>) {
        (internal::find_traits<Name, Is>(tuple, result), ...);
    }(tuple, result, index_sequence);
    return result;
}
} // namespace internal
/*! @endcond */

template <tstring Name, typename Tuple>
consteval std::size_t index_of(const Tuple& tuple) noexcept {
    constexpr auto index = index_of<Name, Tuple>(tuple);
    static_assert(
        index <= std::tuple_size_v<Tuple>,
        "there is no member named as expceted.");
    return index;
}

} // namespace neutron
