// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include "neutron/detail/ecs/construct_from_world.hpp"
#include "neutron/detail/ecs/fwd.hpp"
#include "neutron/detail/ecs/world_accessor.hpp"

namespace neutron {

namespace _insertion {

template <typename Ty>
using _base_t = std::remove_cvref_t<Ty>;

template <typename Ty>
using _request_ptr_t = std::add_pointer_t<std::remove_reference_t<Ty>>;

template <typename...>
struct _all_unique : std::true_type {};

template <typename Head, typename... Tail>
struct _all_unique<Head, Tail...> : std::bool_constant<
    (!(std::same_as<Head, Tail>) && ...) && _all_unique<Tail...>::value> {};

template <typename Requested>
inline constexpr int _request_token_storage = 0;

template <typename Requested>
[[nodiscard]] constexpr auto _request_token() noexcept -> const void* {
    return &_request_token_storage<Requested>;
}

template <typename BoundPtr>
using _bound_base_t =
    std::remove_cv_t<std::remove_pointer_t<std::remove_cvref_t<BoundPtr>>>;

} // namespace _insertion

class insertion_context {
    using _lookup_fn = const void* (*)(const void*, const void*) noexcept;

public:
    constexpr insertion_context() noexcept = default;

    template <typename Requested>
    [[nodiscard]] auto try_get() const noexcept
        -> _insertion::_request_ptr_t<Requested> {
        static_assert(
            std::is_lvalue_reference_v<Requested>,
            "insertion arguments must be references");

        if (lookup_ == nullptr) {
            return nullptr;
        }

        using value_type = std::remove_reference_t<Requested>;
        using const_ptr  = std::add_pointer_t<std::add_const_t<value_type>>;

        auto* ptr = static_cast<const_ptr>(
            lookup_(state_, _insertion::_request_token<Requested>()));
        if constexpr (std::is_const_v<value_type>) {
            return ptr;
        } else {
            return const_cast<value_type*>(ptr);
        }
    }

    template <typename Requested>
    [[nodiscard]] decltype(auto) get() const {
        auto* ptr = try_get<Requested>();
        if (ptr == nullptr) {
            throw std::logic_error("Requested ECS insertion is not bound");
        }
        return static_cast<Requested>(*ptr);
    }

private:
    constexpr insertion_context(const void* state, _lookup_fn lookup) noexcept
        : state_(state), lookup_(lookup) {}

    const void* state_ = nullptr;
    _lookup_fn lookup_ = nullptr;

    template <typename... Bound>
    friend class insertion_bindings;
};

template <typename... Bound>
class insertion_bindings {
    static_assert(
        _insertion::_all_unique<_insertion::_base_t<Bound>...>::value,
        "insertion bindings cannot contain duplicate underlying types");

    using _state_type =
        std::tuple<std::add_pointer_t<std::remove_reference_t<Bound>>...>;

public:
    constexpr explicit insertion_bindings(Bound&&... bound) noexcept
        : state_(std::addressof(bound)...) {}

    [[nodiscard]] constexpr auto context() const noexcept -> insertion_context {
        return insertion_context{ &state_, &_lookup };
    }

private:
    template <typename Ptr>
    static constexpr auto _match_request(const void* token, Ptr ptr) noexcept
        -> const void* {
        using base_type = _insertion::_bound_base_t<Ptr>;

        if (token == _insertion::_request_token<base_type&>()) {
            if constexpr (
                std::is_convertible_v<std::remove_cvref_t<Ptr>, base_type*>) {
                return ptr;
            }
        } else if (token == _insertion::_request_token<const base_type&>()) {
            if constexpr (std::is_convertible_v<
                              std::remove_cvref_t<Ptr>, const base_type*>) {
                return ptr;
            }
        }

        return nullptr;
    }

    static auto _lookup(const void* raw_state, const void* token) noexcept
        -> const void* {
        const auto& state = *static_cast<const _state_type*>(raw_state);
        const void* result = nullptr;

        std::apply(
            [&](auto... ptrs) {
                ((result = result != nullptr ? result
                                             : _match_request(token, ptrs)),
                 ...);
            },
            state);

        return result;
    }

    _state_type state_;
};

template <typename... Bound>
[[nodiscard]] constexpr auto make_insertion_bindings(Bound&&... bound) noexcept(
    std::is_nothrow_constructible_v<
        insertion_bindings<Bound&&...>, Bound&&...>) {
    return insertion_bindings<Bound&&...>{ std::forward<Bound>(bound)... };
}

template <typename... Args>
class insertion : public std::tuple<Args...> {
    using _tuple_base = std::tuple<Args...>;

    static_assert(
        (std::is_lvalue_reference_v<Args> && ...),
        "insertion<T...> requires reference arguments");

    template <world World>
    static auto _context(World& world) -> const insertion_context& {
        auto* ctx = world_accessor::insertion_context(world);
        if (ctx == nullptr) {
            throw std::logic_error("No ECS insertion context is currently bound");
        }
        return *ctx;
    }

public:
    template <world World>
    explicit insertion(World& world)
        : _tuple_base(_context(world).template get<Args>()...) {}
};

template <auto Sys, size_t Index, typename... Args>
struct construct_from_world_t<Sys, insertion<Args...>, Index> {
    template <world World>
    insertion<Args...> operator()(World& world) const {
        return insertion<Args...>{ world };
    }
};

namespace internal {

template <typename>
struct _is_insertion : std::false_type {};

template <typename... Args>
struct _is_insertion<insertion<Args...>> : std::true_type {};

} // namespace internal

} // namespace neutron

template <typename... Args>
struct std::tuple_size<neutron::insertion<Args...>> :
    std::integral_constant<size_t, sizeof...(Args)> {};

template <size_t Index, typename... Args>
struct std::tuple_element<Index, neutron::insertion<Args...>> {
    using type = std::tuple_element_t<Index, std::tuple<Args...>>;
};
