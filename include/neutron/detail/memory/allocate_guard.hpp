// IWYU pragma: private, include <neutron/memory.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <concepts>
    #include <cstddef>
    #include <memory>
    #include <tuple>
    #include <type_traits>
    #include <utility>
#endif
#include "neutron/detail/memory/rebind_alloc.hpp"
#include "neutron/detail/utility/macros.hpp"
#include "neutron/detail/utility/spreader.hpp"

namespace neutron {

namespace internal {

template <typename Alloc, typename IndexSequence, typename... Args>
class _allocate_guard_impl;

template <typename Alloc, std::size_t... Is, typename... Args>
class _allocate_guard_impl<Alloc, std::index_sequence<Is...>, Args...> {
    template <typename T>
    using _alloc_t = rebind_alloc_t<Alloc, T>;

    template <typename T>
    using _traits_t = std::allocator_traits<_alloc_t<T>>;

    static_assert(
        (std::is_pointer_v<typename _traits_t<Args>::pointer> && ...),
        "the allocate result of the allocator type in allocate_guard should not be fancy pointer");

    using _tup_t = std::tuple<Args*...>;

    template <std::size_t I>
    using _elem_t = std::remove_pointer_t<std::tuple_element_t<I, _tup_t>>;

    template <std::size_t I>
    using _alloc_of_t = _alloc_t<_elem_t<I>>;

    template <std::size_t I>
    using _traits_of_t = _traits_t<_elem_t<I>>;

    template <typename Al>
    constexpr _allocate_guard_impl(
        std::allocator_arg_t, const Al& alloc,
        std::size_t
            n) noexcept(std::is_nothrow_constructible_v<Alloc, const Al&>)
        : n_(n), alloc_(alloc), ptrs_() {}

public:
    template <std::convertible_to<Alloc> Al = Alloc>
    ATOM_CONSTEXPR_SINCE_CXX20
        _allocate_guard_impl(std::size_t n, const Al& alloc = {})
        : _allocate_guard_impl(std::allocator_arg, alloc, n) {
        // The target constructor has completed, so if an allocation throws
        // the destructor returns the ones made before it.
        auto allocate = [this]<std::size_t I>(value_spreader<I>) {
            _alloc_of_t<I> al{ alloc_ };
            std::get<I>(ptrs_) = _traits_of_t<I>::allocate(al, n_);
        };
        (allocate(spread_arg<Is>), ...);
    }

    _allocate_guard_impl(const _allocate_guard_impl&)            = delete;
    _allocate_guard_impl(_allocate_guard_impl&&)                 = delete;
    _allocate_guard_impl& operator=(const _allocate_guard_impl&) = delete;
    _allocate_guard_impl& operator=(_allocate_guard_impl&&)      = delete;

    ATOM_CONSTEXPR_SINCE_CXX20 ~_allocate_guard_impl() {
        auto deallocate = [this]<std::size_t I>(value_spreader<I>) {
            _alloc_of_t<I> al{ alloc_ };
            auto p = std::get<I>(ptrs_);
            if (p != nullptr) {
                _traits_of_t<I>::deallocate(al, p, n_);
            }
        };
        (deallocate(spread_arg<Is>), ...);
    }

    /// @brief Returns the I-th allocation; the guard still owns it.
    template <std::size_t I>
    ATOM_ATTRS(ATOM_NODISCARD) constexpr auto get() const& noexcept {
        return std::get<I>(ptrs_);
    }

    /// @brief Takes ownership of the I-th allocation.
    template <std::size_t I>
    ATOM_ATTRS(ATOM_NODISCARD) constexpr auto get() && noexcept {
        return std::exchange(std::get<I>(ptrs_), nullptr);
    }

private:
    std::size_t n_;
    ATOM_NO_UNIQUE_ADDR Alloc alloc_;
    _tup_t ptrs_;
};

} // namespace internal

ATOM_EXPORT template <typename Alloc, typename... Args>
struct allocate_guard :
    internal::_allocate_guard_impl<
        rebind_alloc_t<Alloc, std::byte>, std::index_sequence_for<Args...>,
        Args...> {
    using _isequence_t = std::index_sequence_for<Args...>;
    using _impl_t      = internal::_allocate_guard_impl<
             rebind_alloc_t<Alloc, std::byte>, _isequence_t, Args...>;

    using _impl_t::_impl_t;
};

/**
 * @brief Allocates `n` objects of each of `Args...` from `alloc`.
 *
 * Allocations not taken by `get() &&` return to the allocator when the guard
 * is destroyed, including when a later allocation throws.
 */
ATOM_EXPORT template <
    typename... Args, typename Alloc = std::allocator<std::byte>>
ATOM_ATTRS(ATOM_NODISCARD)
ATOM_CONSTEXPR_SINCE_CXX20
    auto make_allocate_guard(std::size_t n, const Alloc& alloc = {}) {
    return allocate_guard<Alloc, Args...>(n, alloc);
}

/**
 * @brief Moves every allocation of the guard into `ptrs`.
 */
ATOM_EXPORT template <typename... Args, typename Alloc>
ATOM_CONSTEXPR_SINCE_CXX20 void apply_allocation(
    allocate_guard<Alloc, Args...>& guard, Args*&... ptrs) noexcept {
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        ((ptrs = std::move(guard).template get<Is>()), ...);
    }(std::index_sequence_for<Args...>());
}

} // namespace neutron

namespace std {

template <typename Alloc, typename... Args>
struct tuple_size<::neutron::allocate_guard<Alloc, Args...>> {
    static constexpr std::size_t value = sizeof...(Args);
};

template <std::size_t I, typename Alloc, typename... Args>
struct tuple_element<I, ::neutron::allocate_guard<Alloc, Args...>> {
    using type = std::tuple_element_t<I, std::tuple<Args*...>>;
};

} // namespace std
