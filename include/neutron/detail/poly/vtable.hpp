// IWYU pragma: private, include <neutron/polymorphic.hpp>
#pragma once
#include <cstddef>
#include <tuple>
#include <type_traits>
#include "neutron/metafn.hpp"
#include "../macros.hpp"
#include "../member_function_traits.hpp"
#include "../utility/spreader.hpp"
#include "./concepts.hpp"

namespace neutron {

template <poly_object Object, size_t Builtin = sizeof(void*) << 1>
class vtable;

namespace internal {

template <poly_object Object>
class _vtable {
    friend class vtable<Object>;

    template <typename T>
    using _interface   = typename Object::template interface<T>;
    using interface    = _interface<poly_empty_impl>;
    using empty_values = typename Object::template impl<interface>;

    constexpr static size_t size = value_list_size_v<empty_values>;

    // value_list<&impl::foo>; -> type_list<Ret(*)(void*,...);

    template <typename Fn>
    using _void_t = typename member_function_traits<Fn>::void_type;

    template <auto... Values>
    consteval static auto _deduce(value_list<Values...>) noexcept {
        return static_cast<std::tuple<_void_t<decltype(Values)>...>*>(nullptr);
    }

public:
    constexpr _vtable() noexcept = default;

    template <auto... Fns>
    constexpr _vtable(value_list<Fns...> vl) noexcept
        : values(tuple_from_value(vl)) {}

private:
    std::remove_pointer_t<decltype(_deduce(empty_values{}))> values;
};

template <poly_object Object, auto... Fns>
consteval auto make_vtable([[maybe_unused]] value_list<Fns...>)
    -> _vtable<Object> {
    return value_list<member_fn<Fns>::void_fn...>{};
}

} // namespace internal

template <poly_object Object, poly_impl<Object> Impl>
consteval auto make_vtable() noexcept {
    using impl = typename Object::template impl<Impl>;
    return internal::make_vtable<Object>(impl{});
}

template <poly_object Object, poly_impl<Object> Impl>
inline constexpr static auto glob_vtable = make_vtable<Object, Impl>();

template <poly_object Object, size_t Builtin>
class vtable {
public:
    constexpr vtable() noexcept = default;

    template <poly_impl<Object> Impl>
    constexpr vtable([[maybe_unused]] type_spreader<Impl>) noexcept
        : vtable_(make_vtable<Object, Impl>()) {}

    template <size_t Index>
    ATOM_NODISCARD constexpr auto get() const noexcept {
        return std::get<Index>(vtable_.values);
    }

private:
    internal::_vtable<Object> vtable_;
};

template <poly_object Object, size_t Builtin>
requires(sizeof(internal::_vtable<Object>) > Builtin)
class vtable<Object, Builtin> {
public:
    constexpr vtable() noexcept : vptr_() {}

    template <poly_impl<Object> Impl>
    constexpr vtable([[maybe_unused]] type_spreader<Impl>) noexcept
        : vptr_(&glob_vtable<Object, Impl>) {}

    template <size_t Index>
    ATOM_NODISCARD constexpr auto get() const noexcept {
        return std::get<Index>(vptr_->values);
    }

private:
    const internal::_vtable<Object>* vptr_;
};

} // namespace neutron
