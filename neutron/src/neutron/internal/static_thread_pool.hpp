#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include "neutron/execution.hpp"
#include "neutron/lifo_queue.hpp"
#include "./macros.hpp"

namespace neutron {

struct lifo_params {
    size_t num_blocks = 32;
    size_t block_size = 8;
};

struct numa_policy {
    union _inline_buffer;

    const struct _vtable {
        void (*destroy)(_inline_buffer*);
        void (*copy)(_inline_buffer*, const _inline_buffer*);
        void (*move)(_inline_buffer*, _inline_buffer*);
        uint32_t (*get_node)(const _inline_buffer*, uint32_t);
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
                    },
                .get_node = [](const _inline_buffer* policy,
                               uint32_t tid) -> uint32_t {
                    return reinterpret_cast<const Policy*>(&policy->builtin)
                        ->get_node(tid);
                }
            };
        } else {
            return _vtable{
                .destroy =
                    [](_inline_buffer* policy) {
                        delete static_cast<Policy*>(policy->ptr);
                    },
                .copy =
                    [](_inline_buffer* lhs, const _inline_buffer* rhs) {
                        *static_cast<Policy*>(lhs->ptr) =
                            *static_cast<Policy*>(rhs->ptr);
                    },
                .move =
                    [](_inline_buffer* lhs, _inline_buffer* rhs) {
                        std::exchange(lhs->ptr, rhs->ptr);
                    },
                .get_node = [](const _inline_buffer* policy,
                               uint32_t tid) -> uint32_t {
                    return static_cast<Policy*>(policy->ptr)->get_node(tid);
                }
            };
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

    NODISCARD uint32_t get_node(uint32_t logical_tid) const {
        return vtable->get_node(&storage, logical_tid);
    }
};

struct no_numa_policy {
public:
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    NODISCARD uint32_t get_node(uint32_t tid) const noexcept { return 0; }
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
    explicit numa_allocator(uint32_t = 0U) noexcept = default;

    template <typename T>
    explicit numa_allocator(const numa_allocator<T>& that) noexcept {}
};

using default_numa_policy = no_numa_policy;

#endif

inline auto get_numa_policy() -> numa_policy {
    return numa_policy{ default_numa_policy{} };
}

namespace _thread_pool {

using namespace execution;

template <typename Alloc>
struct default_context_maker {
    void operator()([[maybe_unused]] const Alloc&) noexcept {}
};

template <
    template <typename> typename ExternalContextMaker = default_context_maker,
    typename Alloc = numa_allocator<std::byte>>
class _static_thread_pool;

template <typename Alloc>
class _static_thread_pool<default_context_maker, Alloc> {
public:
    _static_thread_pool();
    _static_thread_pool(
        uint32_t threads, numa_policy policy = get_numa_policy())
        : policy_(std::move(policy)) {}

private:
    numa_policy policy_;
};

template <template <typename> typename ExternalContextMaker, typename Alloc>
requires std::is_invocable_v<void, ExternalContextMaker<Alloc>, const Alloc&> &&
         std::is_void_v<
             std::invoke_result_t<ExternalContextMaker<Alloc>, const Alloc&>>
class _static_thread_pool<ExternalContextMaker, Alloc> :
    public _static_thread_pool<default_context_maker, Alloc> {};

template <template <typename> typename ExternalContextMaker, typename Alloc>
requires std::is_invocable_v<ExternalContextMaker<Alloc>, const Alloc&> &&
         (!std::is_void_v<
             std::invoke_result_t<ExternalContextMaker<Alloc>, const Alloc&>>)
class _static_thread_pool<ExternalContextMaker, Alloc> {
    using invoke_result_t =
        std::invoke_result_t<ExternalContextMaker<Alloc>, Alloc>;

    static_assert(
        !std::is_const_v<invoke_result_t>, "context could not be const");

    template <typename T>
    using _allocator_t = rebind_alloc_t<Alloc, T>;

    struct _task_base {};

    struct _thread_state_base {
        explicit _thread_state_base(
            uint32_t index, const numa_policy& numa) noexcept
            : index(index), numa_node(numa.get_node(index)) {}
        uint32_t index;
        uint32_t numa_node;
    };

    class _thread_state : _thread_state_base {
    public:
        template <typename Al>
        explicit _thread_state(
            _static_thread_pool* pool, uint32_t index, lifo_params params,
            const numa_policy& numa, const Al& alloc)
            : _thread_state_base(index, numa),
              local_queue_(params.num_blocks, params.block_size, alloc) {}

    private:
        lifo_queue<_task_base*, _allocator_t<_task_base*>> local_queue_;
    };

public:
    using allocator_type = _allocator_t<std::byte>;
    using alloc_traits   = std::allocator_traits<Alloc>;
    using context_type   = invoke_result_t;

    struct domain : default_domain {};

    class scheduler {
        class sender {
            struct env {
                _static_thread_pool& pool_;

                template <typename Cpo>
                auto query(get_completion_scheduler_t<Cpo>) const noexcept {
                    return _static_thread_pool::scheduler{ pool_ };
                }
            };

        public:
            sender(_static_thread_pool& pool) : pool_(pool) {}

            NODISCARD auto get_env() const noexcept -> env {
                return env{ .pool_ = pool_ };
            }

        private:
            _static_thread_pool& pool_;
        };

    public:
        scheduler(_static_thread_pool& pool) : pool_(&pool) {}

        NODISCARD sender schedule() { return sender{ *pool_ }; }

        NODISCARD auto query(get_forward_progress_guarantee_t) const noexcept
            -> forward_progress_guarantee {
            return forward_progress_guarantee::parallel;
        }

        NODISCARD domain query(get_domain_t) const noexcept { return {}; }

    private:
        _static_thread_pool* pool_;
    };

    template <typename Al = Alloc>
    _static_thread_pool(
        uint32_t num_threads = _hardware_concurrency(), lifo_params params = {},
        numa_policy policy = get_numa_policy(), const Al& alloc = {})
        : thread_count_(num_threads), threads_(num_threads),
          policy_(std::move(policy)) {
        assert(num_threads > 0);

        for (uint32_t i = 0; i < num_threads; ++i) {
            thread_states_[i].emplace(this);
        }
    }

    ~_static_thread_pool() noexcept {
        request_stop();
        join();
    }

    void request_stop() noexcept {
        for (auto& state : thread_states_) {
            state->request_stop();
        }
    }

    auto get_scheduler() -> scheduler { return scheduler{ *this }; }

private:
    static uint32_t _hardware_concurrency() noexcept {
        uint32_t num = std::thread::hardware_concurrency();
        return num == 0 ? 1 : num;
    }

    uint32_t thread_count_;
    std::vector<std::optional<_thread_state>> thread_states_;
    std::vector<std::jthread> threads_;
    numa_policy policy_;
};

} // namespace _thread_pool

using static_thread_pool = _thread_pool::_static_thread_pool<>;

template <
    template <typename> typename CtxMaker,
    typename Alloc = numa_allocator<std::byte>>
using static_context_thread_pool =
    _thread_pool::_static_thread_pool<CtxMaker, Alloc>;

} // namespace neutron
