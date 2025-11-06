#pragma once
#include <exception>
#include <future> // for std::future_error
#include <mutex>
#include "neutron/neutron.hpp"

#if defined(_WIN32)

    #include <atomic>
    #include <memory>
    #include <tuple>
    #include <vector>
    #include <Windows.h>
    #include <ioapiset.h>
    #include <strsafe.h>
    #include <threadpoolapiset.h>

namespace neutron {

namespace internal {

struct result_packet_base {
    void (*deliver)(void*);
    void (*destroy)(void*);
};

template <typename Ty>
struct result_packet;

// completion status
enum completion_status : int8_t {
    terminate = -1,
    finish    = 0
};

/**
 * @internal
 * @class iocp_manager
 * @brief The completion port mamnager, a singleton class.
 * Used to deliver the result of finished tasks.
 */
class iocp_manager {
public:
    static iocp_manager& instance() {
        static iocp_manager instance;

        return instance;
    }

    iocp_manager(const iocp_manager&)            = delete;
    iocp_manager(iocp_manager&&)                 = delete;
    iocp_manager& operator=(const iocp_manager&) = delete;
    iocp_manager& operator=(iocp_manager&&)      = delete;

    ~iocp_manager() {
        _shutdown();
        if (handle_ != nullptr) [[likely]] {
            CloseHandle(handle_);
            handle_ = nullptr;
        }
    }

    NODISCARD HANDLE completion_port() const noexcept { return handle_; }

private:
    static DWORD WINAPI _dispatch_thread(LPVOID lp_param) {
        auto* self = static_cast<iocp_manager*>(lp_param);

        while (true) {
            DWORD bytes_transferred  = 0;
            ULONG_PTR completion_key = 0;
            LPOVERLAPPED overlapped  = nullptr;

            BOOL result = GetQueuedCompletionStatus(
                self->handle_, &bytes_transferred, &completion_key, &overlapped, INFINITE);

            if (completion_key == completion_status::terminate) {
                break;
            }

            // err

            if ((result == 0) || (overlapped == nullptr)) {
                continue;
            }

            // deliver

            auto* packet_base = reinterpret_cast<result_packet_base*>(overlapped);

            packet_base->deliver(packet_base);

            packet_base->destroy(packet_base);
        }

        return 0;
    }

    iocp_manager() : handle_(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0)) {
        if (handle_ == nullptr) [[unlikely]] {
            throw std::system_error(
                static_cast<int>(GetLastError()), std::system_category(), "Failed to create IOCP");
        }

        dispatch_thread_ = CreateThread(nullptr, 0, _dispatch_thread, this, 0, nullptr);

        if (dispatch_thread_ == nullptr) [[unlikely]] {
            CloseHandle(handle_);
            throw std::system_error(
                static_cast<int>(GetLastError()), std::system_category(),
                "Failed to create dispatch thread");
        }
    }

    void _shutdown() {
        PostQueuedCompletionStatus(
            handle_, 0, /*TERMINATE_KEY*/ completion_status::terminate, nullptr);
        if (dispatch_thread_ != nullptr) [[likely]] {
            WaitForSingleObject(dispatch_thread_, INFINITE);
            CloseHandle(dispatch_thread_);
            dispatch_thread_ = nullptr;
        }
    }

    HANDLE handle_;
    HANDLE dispatch_thread_;
};

} // namespace internal

enum class _future_state : uint8_t {
    pending,
    ready,
    exception,
    consumed
};

struct _future_base {
    _future_base() noexcept = default;

    _future_state state() const noexcept { return state_.load(std::memory_order_acquire); }

    void mark_ready() noexcept {
        state_.store(_future_state::ready, std::memory_order_release);
        condvar_.notify_all();
    }

    void mark_exception() noexcept {
        state_.store(_future_state::exception, std::memory_order_release);
        condvar_.notify_all();
    }

    void mark_consumed() noexcept {
        state_.store(_future_state::consumed, std::memory_order_release);
        condvar_.notify_all();
    }

    bool wait_impl() const {
        if (state_.load(std::memory_order_acquire) != _future_state::pending) {
            return true;
        }

        std::unique_lock<std::mutex> ulock(mutex_);
        condvar_.wait(ulock, [this]() {
            return state_.load(std::memory_order_acquire) != _future_state::pending;
        });

        return true;
    }

    template <typename Rep, typename Period>
    bool wait_for_impl(const std::chrono::duration<Rep, Period>& timeout) const {
        if (state_.load(std::memory_order_acquire) != _future_state::pending) {
            return true;
        }

        std::unique_lock<std::mutex> ulock(mutex_);
        return condvar_.wait_for(ulock, timeout, [this]() {
            return state_.load(std::memory_order_acquire) != _future_state::pending;
        });
    }

private:
    std::atomic<_future_state> state_{ _future_state::pending };
    mutable std::mutex mutex_;
    mutable std::condition_variable condvar_;
};

template <typename Ty>
class future : private _future_base {
    friend struct internal::result_packet<Ty>;

public:
    future() noexcept = default;

    NODISCARD Ty get() {
        if (state() == _future_state::consumed) {
            throw std::future_error(std::future_errc::no_state);
        }

        wait_impl();

        if (state() == _future_state::exception) {
            std::rethrow_exception(exception_);
        }

        mark_consumed();
        return std::move(result_);
    }

    void wait() const { wait_impl(); }

    NODISCARD bool valid() const noexcept { return state() != _future_state::consumed; }

    template <typename Rep, typename Period>
    bool wait_for(const std::chrono::duration<Rep, Period>& timeout) const {
        return wait_for_impl(timeout);
    }

    /// @internal
    void set_value(Ty&& value) {
        result_ = std::move(value);
        mark_ready();
    }

private:
    Ty result_;
    std::exception_ptr exception_;
};

template <>
class future<void> : private _future_base {
    friend struct internal::result_packet<void>;

public:
    future() noexcept = default;

    void get() {
        if (state() == _future_state::consumed) [[unlikely]] {
            throw std::future_error(std::future_errc::no_state);
        }

        wait_impl();

        if (state() == _future_state::exception) [[unlikely]] {
            std::rethrow_exception(exception_);
        }

        mark_consumed();
    }

    void wait() const { wait_impl(); }

    NODISCARD bool valid() const noexcept { return state() != _future_state::consumed; }

    template <typename Rep, typename Period>
    bool wait_for(const std::chrono::duration<Rep, Period>& timeout) const {
        return wait_for_impl(timeout);
    }

    void set_value() { mark_ready(); }

private:
    std::exception_ptr exception_;
};

namespace internal {

template <typename Ty>
struct result_packet {
    static void deliver(void* self) {
        auto* packet = static_cast<result_packet*>(self);
        if (packet->exception) {
            packet->future->mark_exception();
        } else {
            packet->future->set_value(std::move(packet->value));
        }
    }

    static void destroy(void* self) {
        auto* packet = static_cast<result_packet*>(self);
        packet->~result_packet();
        ::operator delete(packet);
    }

    result_packet_base base{ .deliver = &deliver, .destroy = &destroy };
    std::shared_ptr<future<Ty>> future;
    Ty value;
    std::exception_ptr exception;
};

template <>
struct result_packet<void> {
    static void deliver(void* self) {
        auto* packet = static_cast<result_packet*>(self);
        if (packet->exception) {
            packet->future->mark_exception();
        } else {
            packet->future->set_value();
        }
    }

    static void destroy(void* self) {
        auto* packet = static_cast<result_packet*>(self);
        packet->~result_packet();
        ::operator delete(packet);
    }

    result_packet_base base{ .deliver = &deliver, .destroy = &destroy };
    std::shared_ptr<future<void>> future;
    std::exception_ptr exception;
};

} // namespace internal

class thread_pool {
public:
    thread_pool(const size_t num_threads = std::thread::hardware_concurrency())
        : pool_(CreateThreadpool(nullptr)) {
        static std::once_flag flag;
        std::call_once(flag, []() {
            internal::iocp_manager::instance(); // make sure initialize iocp
        });

        SetThreadpoolThreadMinimum(pool_, 1);
        SetThreadpoolThreadMaximum(pool_, num_threads);

        InitializeThreadpoolEnvironment(&env_);
        SetThreadpoolCallbackPool(&env_, pool_);
    }

    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&)      = delete;

    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool& operator=(thread_pool&&)      = delete;

    ~thread_pool() noexcept {
        DestroyThreadpoolEnvironment(&env_);
        CloseThreadpool(pool_);
    }

private:
    template <typename Callable, typename Ret, typename... Args>
    struct task_context {
        Callable callable;
        std::tuple<Args...> args;
        std::shared_ptr<future<Ret>> future;

        static Ret call(task_context* context) {
            return std::apply(context->callable, std::move(context->args));
        }
    };

    template <typename Callable, typename Ret>
    struct task_context<Callable, Ret> {
        Callable callable;
        std::shared_ptr<future<Ret>> future;

        static Ret call(task_context* context) { return context->callable(); }
    };

    template <typename Callable, typename Ret, typename... Args>
    static task_context<Callable, Ret, Args...>*
        _create_task(Callable&& callable, std::shared_ptr<future<Ret>> fut, Args&&... args) {
        auto context = static_cast<task_context<Callable, Ret, Args...>*>(
            ::operator new(sizeof(task_context<Callable, Ret, Args...>)));

        ::new (&context->callable) std::remove_cvref_t<Callable>(std::forward<Callable>(callable));
        if constexpr (sizeof...(Args)) {
            ::new (&context->args) std::tuple<Args...>(std::forward<Args>(args)...);
        }
        ::new (&context->future) std::shared_ptr<future<Ret>>(std::move(fut));
        return context;
    }

    template <typename Callable, typename Ret, typename... Args>
    static void _destroy_task(task_context<Callable, Ret, Args...>* context) {
        /*std::destroy_at(context->callable);
        context->future.~shared_ptr();*/
        context->~task_context();
        ::operator delete(context);
    }

    static void _post_result(std::shared_ptr<future<void>> future) {
        auto* packet = static_cast<internal::result_packet<void>*>(
            ::operator new(sizeof(internal::result_packet<void>)));

        ::new (packet) internal::result_packet<void>{ .future = std::move(future) };

        PostQueuedCompletionStatus(
            internal::iocp_manager::instance().completion_port(), 0,
            internal::completion_status::finish, reinterpret_cast<LPOVERLAPPED>(packet));
    }

    template <typename Ty>
    static void _post_result(std::shared_ptr<future<Ty>> future, Ty&& value) {
        auto packet = static_cast<internal::result_packet<Ty>*>(
            ::operator new(sizeof(internal::result_packet<Ty>)));

        ::new (packet) internal::result_packet<Ty>{ .future = std::move(future),
                                                    .value  = std::forward<Ty>(value) };

        PostQueuedCompletionStatus(
            internal::iocp_manager::instance().completion_port(), 0,
            internal::completion_status::finish, reinterpret_cast<LPOVERLAPPED>(packet));
    }

    template <typename Ty>
    static void _post_exception(std::shared_ptr<future<Ty>> future, std::exception_ptr exception) {
        auto packet = static_cast<internal::result_packet<Ty>*>(
            ::operator new(sizeof(internal::result_packet<Ty>)));

        new (packet) internal::result_packet<Ty>{ .future    = std::move(future),
                                                  .exception = std::move(exception) };

        PostQueuedCompletionStatus(
            internal::iocp_manager::instance().completion_port(), 0, 0,
            reinterpret_cast<LPOVERLAPPED>(packet));
    }

    template <typename Callable, typename Ret, typename... Args>
    static void CALLBACK _task_proxy(PTP_CALLBACK_INSTANCE instance, PVOID context) {
        using context_type = task_context<Callable, Ret, Args...>;

        auto* ctx = static_cast<context_type*>(context);
        try {
            if constexpr (std::is_same_v<Ret, void>) {
                context_type::call(ctx);
                _post_result(ctx->future);
            } else {
                Ret result = context_type::call(ctx);
                _post_result(ctx->future, std::move(result));
            }
        } catch (...) {
            _post_exception(ctx->future, std::current_exception());
        }

        _destroy_task(ctx);
    }

public:
    template <typename Callable, typename... Args>
    auto submit(Callable&& callable, Args&&... args) {
        using return_type = std::invoke_result_t<Callable, Args...>;
        using future_type = future<return_type>;

        auto fut      = std::make_shared<future_type>();
        auto* context = _create_task<Callable, return_type, Args...>(
            std::forward<Callable>(callable), fut, std::forward<Args>(args)...);

        if (!TrySubmitThreadpoolCallback(
                &_task_proxy<Callable, return_type, Args...>, context, &env_)) [[unlikely]] {
            _destroy_task<Callable, return_type>(context);
            throw std::runtime_error("Failed to submit task to thread pool");
        }

        return fut;
    }

private:
    PTP_POOL pool_{};
    TP_CALLBACK_ENVIRON env_{};
};

namespace internal {

constexpr auto default_blocks_per_chunk = 64;
template <size_t BlockSize, size_t BlocksPerChunk = default_blocks_per_chunk>
class packet_memory_pool {
public:
    packet_memory_pool() { _allocate_chunk(); }

    packet_memory_pool(const packet_memory_pool&)            = delete;
    packet_memory_pool(packet_memory_pool&&)                 = delete;
    packet_memory_pool& operator=(const packet_memory_pool&) = delete;
    packet_memory_pool& operator=(packet_memory_pool&&)      = delete;

    ~packet_memory_pool() {
        for (auto* chunk : chunks_) {
            ::operator delete(chunk);
        }
    }

    void* allocate() {
        if (free_list_ == nullptr) {
            _allocate_chunk();
        }

        void* block = free_list_;
        free_list_  = *reinterpret_cast<void**>(free_list_);
        return block;
    }

    void deallocate(void* block) {
        *static_cast<void**>(block) = free_list_;
        free_list_                  = block;
    }

private:
    void _allocate_chunk() {
        void* chunk = ::operator new(BlockSize * BlocksPerChunk);
        chunks_.push_back(chunk);

        char* current = static_cast<char*>(chunk);
        for (size_t i = 0; i < BlocksPerChunk; ++i) {
            *reinterpret_cast<void**>(current) = current + BlockSize;
            current += BlockSize;
        }
        *reinterpret_cast<void**>(current) = free_list_;
        free_list_                         = chunk;
    }

    void* free_list_{ nullptr };
    std::vector<void*> chunks_;
};

template <typename Ty>
class result_packet_allocator {
    static packet_memory_pool<sizeof(result_packet<Ty>)>& _pool() {
        static packet_memory_pool<sizeof(result_packet<Ty>)> instance;
        return instance;
    }

public:
    static result_packet<Ty>* create(std::shared_ptr<future<Ty>> future, Ty value) {
        auto packet = static_cast<result_packet<Ty>*>(
            packet_memory_pool<sizeof(result_packet<Ty>)>::allocate());
        new (packet) result_packet<Ty>{ .future = std::move(future), .value = std::move(value) };
        return packet;
    }

    static result_packet<Ty>*
        create_exception(std::shared_ptr<future<Ty>> future, std::exception_ptr exception) {
        auto packet = static_cast<result_packet<Ty>*>(
            packet_memory_pool<sizeof(result_packet<Ty>)>::allocate());
        new (packet) result_packet<Ty>{ .future = std::move(future), .exception = exception };
        return packet;
    }

    static void destroy(result_packet<Ty>* packet) {
        packet->~result_packet();
        _pool().deallocate(packet);
    }
};

} // namespace internal

} // namespace neutron
#else
namespace neutron {

class thread_pool {
public:
    thread_pool(const std::size_t num_threads = std::thread::hardware_concurrency())
        : num_threads_(num_threads) {
        try {
            for (auto i = 0; i < num_threads / 2; ++i) {
                emplace_thread();
            }
        } catch (...) {
            threads_.clear();
            num_threads_ = 0;
            throw;
        }
    }

    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&)      = delete;

    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool& operator=(thread_pool&&)      = delete;

    ~thread_pool() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }

        condvar_.notify_all();

        // jthread
    #if !defined(__cpp_lib_jthread)
        for (auto& thread : threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    #endif
    }

    /**
     * @brief add a new task to the queue.
     *
     * @tparam Callable the type of the task, could be deduced.
     * @tparam Args argument types, could be deduced.
     * @param callable the task.
     * @param args arguments for finish the task.
     * @return future.
     */
    template <typename Callable, typename... Args>
    auto submit(Callable&& callable, Args&&... args)
        -> std::future<std::invoke_result_t<Callable, Args...>> {
        using return_type = std::invoke_result_t<Callable, Args...>;

        if (stop_) [[unlikely]] {
            throw std::runtime_error("submit on stopped thread pool");
        }

        if (threads_.size() ^ num_threads_) {
            emplace_thread();
        }

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<callable>(callable), std::forward<args>(args)...));

        std::future<return_type> future = task->get_future();

        {
            std::unique_lock<std::mutex> lock(mutex_);
            tasks_.emplace([task]() { (*task)(); });
        }
        condvar_.notify_one();

        return future;
    }

private:
    void emplace_thread() {
        threads_.emplace_back([this]() {
            while (true) {
                std::unique_lock<std::mutex> lock(mutex_);
                condvar_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });

                if (stop_ && tasks_.empty()) {
                    return;
                }

                auto task{ std::move(tasks_.front()) };
                tasks_.pop();
                lock.unlock();
                task();
            }
        });
    }

    std::atomic<bool> stop_{ false };
    uint32_t num_threads_;
    std::mutex mutex_;
    #if defined(__cpp_lib_jthread)
    std::vector<std::jthread> threads_;
    #else
    std::vector<std::thread> threads_;
    #endif
    std::condition_variable condvar_;
    std::queue<std::function<void()>> tasks_;
};

} // namespace neutron

#endif