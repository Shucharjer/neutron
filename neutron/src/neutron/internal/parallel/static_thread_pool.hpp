/**
 * @file static_thread_pool.hpp
 * @brief This file is the internal implementation of `static_thread_pool`.
 * It references the implementation in `exec`, but unlike it, it doesn't
 * involve NUMA scheduling because its use case it typically a single NUMA
 * node. Additionally, it supports custom thread context; you only need to
 * provide a suitable `Callable` to construct it via `Alloc`.
 * 
 * @date 2025-11-23
 */
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
#include "../macros.hpp"
#include "./lifo_queue.hpp"

namespace neutron {

struct lifo_params {
    size_t num_blocks = 32;
    size_t block_size = 8;
};

namespace _thread_pool {

using namespace execution;

template <typename Alloc>
struct default_context_maker {
    void operator()([[maybe_unused]] const Alloc&) noexcept {}
};

template <
    template <typename> typename ExternalContextMaker = default_context_maker,
    typename Alloc = std::allocator<std::byte>>
class _static_thread_pool;

template <typename Alloc>
class _static_thread_pool<default_context_maker, Alloc> {
public:
    _static_thread_pool();
    _static_thread_pool(uint32_t threads) {}

private:
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
        explicit _thread_state_base(uint32_t index) noexcept : index(index) {}
        uint32_t index;
    };

    class _thread_state : _thread_state_base {
    public:
        template <typename Al>
        explicit _thread_state(
            _static_thread_pool* pool, uint32_t index, lifo_params params,
            const Al& alloc)
            : _thread_state_base(index),
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
        const Al& alloc = {})
        : thread_count_(num_threads), threads_(num_threads) {
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
};

} // namespace _thread_pool

using static_thread_pool = _thread_pool::_static_thread_pool<>;

template <
    template <typename> typename CtxMaker,
    typename Alloc = std::allocator<std::byte>>
using static_context_thread_pool =
    _thread_pool::_static_thread_pool<CtxMaker, Alloc>;

} // namespace neutron
