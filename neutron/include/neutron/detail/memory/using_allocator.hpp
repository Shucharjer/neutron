// IWYU pragma: private, include <neutron/memory.hpp>
#pragma once
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <neutron/concepts.hpp>
#include <neutron/utility.hpp>
#include "neutron/detail/concepts/allocator.hpp"
#include "neutron/detail/macros.hpp"

namespace neutron {

template <
    std_simple_allocator Alloc,
    typename SizeT = typename std::allocator_traits<Alloc>::size_type>
ATOM_CONSTEXPR_SINCE_CXX20 auto allocate(Alloc& alloc, SizeT n = 1) {
    return std::allocator_traits<Alloc>::allocate(alloc, n);
}

template <
    std_simple_allocator Alloc, typename... Args,
    typename Ty      = typename std::allocator_traits<Alloc>::value_type,
    typename Pointer = typename std::allocator_traits<Alloc>::pointer>
ATOM_CONSTEXPR_SINCE_CXX20 void construct(
    Alloc& alloc, Pointer ptr,
    Args&&... args) noexcept(std::is_nothrow_constructible_v<Ty, Args...>) {
    std::uninitialized_construct_using_allocator(
        std::to_address(ptr), alloc, std::forward<Args>(args)...);
}

template <
    std_simple_allocator Alloc,
    typename Ty      = typename std::allocator_traits<Alloc>::value_type,
    typename Pointer = typename std::allocator_traits<Alloc>::pointer>
ATOM_CONSTEXPR_SINCE_CXX20 void destroy(Alloc& alloc, Pointer ptr) noexcept(
    std::is_nothrow_destructible_v<Ty>) {
    return std::allocator_traits<Alloc>::destroy(alloc, ptr);
}

template <
    std_simple_allocator Alloc,
    typename Pointer = typename std::allocator_traits<Alloc>::pointer>
ATOM_CONSTEXPR_SINCE_CXX20 void
    deallocate(Alloc& alloc, Pointer ptr, size_t n = 1) noexcept {
    return std::allocator_traits<Alloc>::deallocate(alloc, ptr, n);
}

namespace _default_construct_using_allocator {

template <typename Alloc, typename Ty, bool>
struct _construct {
    using pointer   = std::allocator_traits<Alloc>::pointer;
    using size_type = std::allocator_traits<Alloc>::size_type;

    static ATOM_CONSTEXPR_SINCE_CXX20 auto construct_n(
        [[maybe_unused]] Alloc& alloc, pointer ptr,
        size_type n) noexcept(std::is_nothrow_default_constructible_v<Ty>) {
        size_type curr = 0;
        ATOM_TRY {
            for (; curr < n; ++curr) {
                ::new (std::to_address(ptr + curr)) Ty;
            }
        }
        ATOM_CATCH(...) {
            for (size_type i = curr; i-- > 0;) {
                std::destroy_at(std::to_address(ptr + i));
            }
        }
        return ptr + n;
    }
};

template <typename Alloc, typename Ty>
struct _construct<Alloc, Ty, true> {
    using pointer   = std::allocator_traits<Alloc>::pointer;
    using size_type = std::allocator_traits<Alloc>::size_type;

    static ATOM_CONSTEXPR_SINCE_CXX20 auto
        construct_n([[maybe_unused]] Alloc& alloc, pointer ptr, size_type n) {
        size_type curr = 0;
        ATOM_TRY {
            for (; curr != n; ++curr) {
                std::allocator_traits<Alloc>::construct(
                    alloc, std::to_address(ptr + curr));
            }
        }
        ATOM_CATCH(...) {
            for (size_type i = curr; i-- > 0;) {
                std::allocator_traits<Alloc>::destroy(
                    alloc, std::to_address(ptr + i));
            }
            ATOM_RETHROW;
        }
        return ptr + n;
    }
};

} // namespace _default_construct_using_allocator

template <
    std_simple_allocator Alloc,
    typename Ty      = std::allocator_traits<Alloc>::value_type,
    typename Pointer = std::allocator_traits<Alloc>::pointer,
    typename SizeT   = std::allocator_traits<Alloc>::size_type>
ATOM_CONSTEXPR_SINCE_CXX20 Pointer
    uninitialized_default_construct_n_using_allocator(
        Alloc& alloc, Pointer ptr, SizeT n) {
    constexpr bool uses_allocator = std::uses_allocator_v<Ty, Alloc>;
    return _default_construct_using_allocator::_construct<
        Alloc, Ty, uses_allocator>::construct_n(alloc, ptr, n);
}

template <
    std_simple_allocator Alloc,
    typename Ty      = std::allocator_traits<Alloc>::value_type,
    typename Pointer = std::allocator_traits<Alloc>::pointer,
    typename SizeT   = std::allocator_traits<Alloc>::size_type>
ATOM_CONSTEXPR_SINCE_CXX20 Pointer
    uninitialized_value_construct_n_using_allocator(
        Alloc& alloc, Pointer ptr, SizeT n) {
    constexpr bool uses_allocator = std::uses_allocator_v<Ty, Alloc>;
    // depends on the internal behaviour of
    // std::allocator_traits<Alloc>::construct
    return _default_construct_using_allocator::_construct<
        Alloc, Ty, true>::construct_n(alloc, ptr, n);
}

template <
    std_simple_allocator Alloc,
    typename Ty      = std::allocator_traits<Alloc>::value_type,
    typename Pointer = std::allocator_traits<Alloc>::pointer,
    typename SizeT   = std::allocator_traits<Alloc>::size_type>
ATOM_CONSTEXPR_SINCE_CXX20 Pointer uninitialized_fill_n_using_allocator(
    Alloc& alloc, Pointer ptr, SizeT n, const Ty& value) {
    SizeT curr = 0;
    ATOM_TRY {
        for (; curr != n; ++curr) {
            std::allocator_traits<Alloc>::construct(
                alloc, std::to_address(ptr + curr));
        }
    }
    ATOM_CATCH(...) {
        for (SizeT i = curr; i-- > 0;) {
            std::allocator_traits<Alloc>::destroy(
                alloc, std::to_address(ptr + i));
        }
        ATOM_RETHROW;
    }
    return ptr + n;
}

template <
    std_simple_allocator Alloc, typename InputIter,
    typename Ty      = std::allocator_traits<Alloc>::value_type,
    typename Pointer = std::allocator_traits<Alloc>::pointer,
    typename SizeT   = std::allocator_traits<Alloc>::size_type>
ATOM_CONSTEXPR_SINCE_CXX20 Pointer uninitialized_copy_using_allocator(
    Alloc& alloc, InputIter ifirst, InputIter ilast, Pointer ofirst) {
    using traits_t = std::allocator_traits<Alloc>;
    Pointer it     = ofirst;
    ATOM_TRY {
        for (; ifirst != ilast; ++ifirst, ++it) {
            traits_t::construct(alloc, std::to_address(it), *ifirst);
        }
    }
    ATOM_CATCH(...) {
        for (; ofirst != it; ++ofirst) {
            traits_t::destroy(alloc, std::to_address(ofirst));
        }
        ATOM_RETHROW;
    }
    return it;
}

template <
    std_simple_allocator Alloc,
    typename Ty        = std::allocator_traits<Alloc>::value_type,
    typename Pointer   = std::allocator_traits<Alloc>::pointer,
    typename InputIter = Pointer,
    typename SizeT     = std::allocator_traits<Alloc>::size_type>
ATOM_CONSTEXPR_SINCE_CXX20 Pointer uninitialized_copy_n_using_allocator(
    Alloc& alloc, InputIter src, SizeT n,
    Pointer dst) noexcept(std::
                              is_nothrow_constructible_v<
                                  Ty, decltype(*std::declval<InputIter>())>) {
    using traits_t = std::allocator_traits<Alloc>;
    SizeT curr     = 0;
    InputIter it   = src;
    ATOM_TRY {
        for (; curr != n; ++curr, ++it) {
            traits_t::construct(alloc, std::to_address(dst + curr), *it);
        }
    }
    ATOM_CATCH(...) {
        for (SizeT i = curr; i-- > 0;) {
            traits_t::destroy(alloc, std::to_address(dst + i));
        }
        ATOM_RETHROW;
    }
    return dst + n;
}

template <
    std_simple_allocator Alloc,
    typename Ty        = std::allocator_traits<Alloc>::value_type,
    typename Pointer   = std::allocator_traits<Alloc>::pointer,
    typename InputIter = Pointer,
    typename SizeT     = std::allocator_traits<Alloc>::size_type>
ATOM_CONSTEXPR_SINCE_CXX20 Pointer uninitialized_move_n_using_allocator(
    Alloc& alloc, InputIter src, SizeT n,
    Pointer dst) noexcept(std::
                              is_nothrow_constructible_v<
                                  Ty, decltype(std::move(
                                          *std::declval<InputIter>()))>) {
    using traits_t = std::allocator_traits<Alloc>;
    SizeT curr     = 0;
    InputIter it   = src;
    ATOM_TRY {
        for (; curr != n; ++curr) {
            traits_t::construct(
                alloc, std::to_address(dst + curr), std::move(*(src + curr)));
        }
    }
    ATOM_CATCH(...) {
        for (SizeT i = curr; i-- > 0;) {
            traits_t::destroy(alloc, std::to_address(dst + i));
        }
        ATOM_RETHROW;
    }
    return dst + n;
}

template <
    std_simple_allocator Alloc,
    typename Ty      = std::allocator_traits<Alloc>::value_type,
    typename Pointer = std::allocator_traits<Alloc>::pointer,
    typename SizeT   = std::allocator_traits<Alloc>::size_type>
ATOM_CONSTEXPR_SINCE_CXX20 Pointer
    uninitialized_move_if_noexcept_n_using_allocator(
        Alloc& alloc, Pointer src, SizeT n,
        Pointer dst) noexcept(nothrow_conditional_movable<Ty>) {
    if constexpr (std::is_nothrow_move_constructible_v<Ty>) {
        return uninitialized_move_n_using_allocator(alloc, src, n, dst);
    }
    return uninitialized_copy_n_using_allocator(alloc, src, n, dst);
}

} // namespace neutron
