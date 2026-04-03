// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include "neutron/detail/macros.hpp"
#include "neutron/detail/ecs/construct_from_world.hpp"

namespace neutron {

namespace _global {

template <typename Ty>
using _base_t = std::remove_cvref_t<Ty>;

template <typename...>
struct _all_unique : std::true_type {};

template <typename Head, typename... Tail>
struct _all_unique<Head, Tail...> : std::bool_constant<
    (!(std::same_as<Head, Tail>) && ...) && _all_unique<Tail...>::value> {};

template <typename Ty>
struct _slot {
    static inline std::shared_mutex mutex{};
    static inline _base_t<Ty>* object = nullptr;
};

template <typename Ty>
using _lock_t = std::conditional_t<
    std::is_const_v<std::remove_reference_t<Ty>>,
    std::shared_lock<std::shared_mutex>, std::unique_lock<std::shared_mutex>>;

template <typename Ty>
auto _make_lock() {
    return _lock_t<Ty>{ _slot<_base_t<Ty>>::mutex, std::defer_lock };
}

template <typename... Args>
class _lock_pack {
    using _lock_tuple = std::tuple<_lock_t<Args>...>;

protected:
    _lock_pack() : locks_(_make_lock<Args>()...) { _lock_all(); }

private:
    void _lock_all() {
        if constexpr (sizeof...(Args) == 0) {
            return;
        } else if constexpr (sizeof...(Args) == 1) {
            std::get<0>(locks_).lock();
        } else {
            std::apply([](auto&... locks) { std::lock(locks...); }, locks_);
        }
    }

    _lock_tuple locks_{};
};

template <typename Ty>
_base_t<Ty>& _must_get_bound() {
    auto* ptr = _slot<_base_t<Ty>>::object;
    if (ptr == nullptr) {
        throw std::logic_error("Requested ECS global is not bound");
    }
    return *ptr;
}

template <typename Ty>
decltype(auto) _forward_bound() {
    if constexpr (std::is_reference_v<Ty>) {
        return static_cast<Ty>(_must_get_bound<Ty>());
    } else if constexpr (std::is_const_v<Ty>) {
        return static_cast<const _base_t<Ty>&>(_must_get_bound<Ty>());
    } else {
        return static_cast<Ty>(_must_get_bound<Ty>());
    }
}

} // namespace _global

template <typename Ty>
void bind_global(Ty& object) noexcept {
    _global::_slot<_global::_base_t<Ty>>::object = std::addressof(object);
}

template <typename Ty>
void unbind_global() noexcept {
    _global::_slot<_global::_base_t<Ty>>::object = nullptr;
}

template <typename Ty>
ATOM_NODISCARD bool has_global() noexcept {
    return _global::_slot<_global::_base_t<Ty>>::object != nullptr;
}

template <typename Ty>
ATOM_NODISCARD auto try_get_global() noexcept -> _global::_base_t<Ty>* {
    return _global::_slot<_global::_base_t<Ty>>::object;
}

template <typename Ty>
[[nodiscard]] auto get_global() -> _global::_base_t<Ty>& {
    return _global::_must_get_bound<Ty>();
}

template <typename Ty>
class scoped_global_binding {
    using _base_type = _global::_base_t<Ty>;

public:
    explicit scoped_global_binding(_base_type& object) noexcept
        : previous_(_global::_slot<_base_type>::object) {
        bind_global(object);
    }

    scoped_global_binding(const scoped_global_binding&)            = delete;
    scoped_global_binding& operator=(const scoped_global_binding&) = delete;
    scoped_global_binding(scoped_global_binding&&)                 = delete;
    scoped_global_binding& operator=(scoped_global_binding&&)      = delete;

    ~scoped_global_binding() noexcept {
        _global::_slot<_base_type>::object = previous_;
    }

private:
    _base_type* previous_;
};

template <typename... Args>
class global : private _global::_lock_pack<Args...>, public std::tuple<Args...> {
    using _lock_base  = _global::_lock_pack<Args...>;
    using _tuple_base = std::tuple<Args...>;

    static_assert(
        _global::_all_unique<std::remove_cvref_t<Args>...>::value,
        "global<T...> cannot contain duplicate underlying types");

public:
    template <world World>
    explicit global(World&) : _lock_base(), _tuple_base(_global::_forward_bound<Args>()...) {}
};

template <auto Sys, size_t Index, typename... Args>
struct construct_from_world_t<Sys, global<Args...>, Index> {
    template <world World>
    global<Args...> operator()(World& world) const {
        return global<Args...>(world);
    }
};

namespace internal {

template <typename>
struct _is_global : std::false_type {};

template <typename... Args>
struct _is_global<global<Args...>> : std::true_type {};

} // namespace internal

} // namespace neutron

template <typename... Args>
struct std::tuple_size<neutron::global<Args...>> :
    std::integral_constant<size_t, sizeof...(Args)> {};

template <size_t Index, typename... Args>
struct std::tuple_element<Index, neutron::global<Args...>> {
    using type = std::tuple_element_t<Index, std::tuple<Args...>>;
};
