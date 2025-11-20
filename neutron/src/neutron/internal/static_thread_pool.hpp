#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include "./macros.hpp"

namespace neutron {

struct numa_policy {
    union _inline_buffer;

    const struct _vtable {
        void (*destroy)(_inline_buffer*);
        void (*copy)(_inline_buffer*, const _inline_buffer*);
        void (*move)(_inline_buffer*, _inline_buffer*);
    }* vtable;

    constexpr static size_t builtin_size = sizeof(void*) << 1;
    union _inline_buffer {
        void* ptr;

        std::byte builtin[builtin_size]; // NOLINT

        constexpr _inline_buffer() noexcept = default;

        template <typename Policy>
        requires(sizeof(std::decay_t<Policy>) > builtin_size)
        _inline_buffer(Policy&& policy)
            : ptr(::new std::remove_cvref_t<Policy>(
                  std::forward<Policy>(policy))) {}

        template <typename Policy>
        requires(sizeof(std::decay_t<Policy>) <= builtin_size)
        _inline_buffer(Policy&& policy) : builtin() {
            ::new (&builtin)
                std::remove_cvref_t<Policy>(std::forward<Policy>(policy));
        }
    } storage;

    template <typename Policy>
    constexpr static _vtable _vtable_for() noexcept {
        if constexpr (sizeof(std::decay_t<Policy>) <= builtin_size) {
            return _vtable{
                .destroy =
                    [](_inline_buffer* policy) {
                        reinterpret_cast<Policy*>(&policy->builtin)->~Policy();
                    },
                .copy =
                    [](_inline_buffer* lhs, const _inline_buffer* rhs) {
                        *reinterpret_cast<Policy*>(&lhs->builtin) =
                            *reinterpret_cast<const Policy*>(&rhs->builtin);
                    },
                .move =
                    [](_inline_buffer* lhs, _inline_buffer* rhs) {
                        *reinterpret_cast<Policy*>(&lhs->builtin) = std::move(
                            *reinterpret_cast<Policy*>(&rhs->builtin));
                    }
            };
        } else {
            return _vtable{ .destroy =
                                [](_inline_buffer* policy) {
                                    delete static_cast<Policy*>(policy->ptr);
                                },
                            .copy =
                                [](_inline_buffer* lhs,
                                   const _inline_buffer* rhs) {
                                    *static_cast<Policy*>(lhs->ptr) =
                                        *static_cast<Policy*>(rhs->ptr);
                                },
                            .move =
                                [](_inline_buffer* lhs, _inline_buffer* rhs) {
                                    std::exchange(lhs->ptr, rhs->ptr);
                                } };
        }
    }

    template <typename Policy>
    constexpr static _vtable _glob_vtable =
        _vtable_for<std::remove_cvref_t<Policy>>();

    template <typename Policy>
    requires(!std::same_as<std::remove_cvref_t<Policy>, numa_policy>)
    explicit numa_policy(Policy&& policy)
        : vtable(&_glob_vtable<Policy>), storage(std::forward<Policy>(policy)) {
    }

    numa_policy(const numa_policy& that) : vtable(that.vtable), storage() {
        vtable->copy(&storage, &that.storage);
    }

    numa_policy(numa_policy&& that) noexcept : vtable(that.vtable), storage() {
        vtable->move(&storage, &that.storage);
    }

    numa_policy& operator=(const numa_policy&) = delete;

    numa_policy& operator=(numa_policy&&) = delete;

    ~numa_policy() noexcept { vtable->destroy(&storage); }
};

struct no_numa_policy {
public:
};

#if __has_include(<numa.h>)
    #include <numa.h>

template <typename Ty>
class numa_allocator {
public:
    static_assert(!std::is_const_v<Ty>);
    static_assert(!std::is_volatile_v<Ty>);

    using value_type      = Ty;
    using pointer         = Ty*;
    using const_pointer   = const Ty*;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagete_on_container_swap            = std::true_type;

    explicit numa_allocator(uint32_t node) noexcept : node_(node) {}

    template <typename T>
    explicit numa_allocator(const numa_allocator<T>& that) noexcept
        : node_(that.node_) {}

    ATOM_ALLOCATOR
    NODISCARD CONSTEXPR23 Ty* allocate(size_type n) {
        static_assert(sizeof(Ty) >= 0);
    #if HAS_CXX23
        if consteval {
            return std::allocator<Ty>{}.allocate(n);
        } else {
            auto* const ptr =
                static_cast<Ty*>(::numa_alloc_onnode(n * sizeof(Ty), node_));
            if (ptr == nullptr) [[unlikely]] {
                throw std::bad_alloc();
            }
            return ptr;
        }
    #else
        auto* const ptr =
            static_cast<Ty*>(::numa_alloc_onnode(n * sizeof(Ty), node_));
        if (ptr == nullptr) [[unlikely]] {
            throw std::bad_alloc();
        }
        return ptr;
    #endif
    }

    ATOM_ALLOCATOR
    NODISCARD CONSTEXPR23 Ty*
        allocate(size_type n, [[maybe_unused]] std::nothrow_t) noexcept {
        static_assert(sizeof(Ty) >= 0);
    #if HAS_CXX23
        if consteval {
            return std::allocator<Ty>{}.allocate(n);
        } else {
            return static_cast<Ty*>(::numa_alloc_onnode(n * sizeof(Ty), node_));
        }
    #else
        return static_cast<Ty*>(::numa_alloc_onnode(n * sizeof(Ty), node_));
    #endif
    }

    CONSTEXPR23 void deallocate(Ty* ptr, size_type n) noexcept {
    #if HAS_CXX23
        if consteval {
            std::allocator<Ty>{}.deallocate(ptr, n);
        } else {
            ::numa_free(static_cast<void*>(ptr), n * sizeof(Ty));
        }
    #else
        ::numa_free(static_cast<void*>(ptr), n * sizeof(Ty));
    #endif
    }

    constexpr bool operator==(const numa_allocator& that) noexcept {
    #if HAS_CXX23
        if consteval {
            return true;
        } else {
            return node_ == that.node_;
        }
    #else
        return node_ == that.node_;
    #endif
    }

    template <typename T>
    constexpr bool operator==(const numa_allocator<T>& that) noexcept {
    #if HAS_CXX23
        if consteval {
            return true;
        } else {
            return node_ == that.node_;
        }
    #else
        return node_ == that.node_;
    #endif
    }

private:
    uint32_t node_;
};

struct default_numa_policy {};

#else

template <typename Ty>
class numa_allocator : public std::allocator<Ty> {
public:
    explicit numa_allocator(uint32_t node) noexcept = default;

    template <typename T>
    explicit numa_allocator(const numa_allocator<T>& that) noexcept {}
};

using default_numa_policy = no_numa_policy;

#endif

inline auto get_numa_policy() -> numa_policy {
    return numa_policy{ default_numa_policy{} };
}

template <
    typename ExternalContext = void, typename Alloc = std::allocator<std::byte>>
class _static_thread_pool;

template <typename Alloc>
class _static_thread_pool<void, Alloc> {
public:
};

template <typename ExternalContext, typename Alloc>
class _static_thread_pool {
    numa_policy policy_;

public:
    _static_thread_pool();
    _static_thread_pool(
        uint32_t threads, numa_policy policy = get_numa_policy())
        : policy_(std::move(policy)) {}
};

using static_thread_pool = _static_thread_pool<>;

} // namespace neutron
