#define ATOM_EXECUTION
#include <atomic>
#include <cmath>
#include <cstdint>
#include <neutron/execution.hpp>
#include "neutron/print.hpp"
#include "require.hpp"
#ifdef ATOM_EXECUTION
    #include "thread_pool.hpp"
#elif __has_include(<exec/static_thread_pool.hpp>)
    #include <exec/static_thread_pool.hpp>
using thread_pool = exec::static_thread_pool;
#else
    #error
#endif
#include "timer.hpp"

using namespace neutron;
using namespace neutron::execution;

int main() {

    {
        constexpr auto count      = 1024;
        std::atomic<uint32_t> num = 0;
        sender auto sndr =
            just() | bulk(count, [&num](uint32_t size) { ++num; });
        this_thread::sync_wait(sndr);
        require_or_return(num == count, 1);
    }

    {
        constexpr auto count      = 1024;
        std::atomic<uint32_t> num = 0;

        thread_pool pool{ 4 };
        scheduler auto sch = pool.get_scheduler();
        sender auto sndr =
            schedule(sch) | bulk(count, [&num](uint32_t size) { ++num; });
        this_thread::sync_wait(sndr);
        require_or_return(num == count, 1);
    }

    {
        auto timer = set_timer("single thread");

        constexpr uint64_t count      = 1024UL * 1024 * 1024;
        constexpr auto blocks         = 4;
        constexpr uint64_t chunk_size = count / blocks;
        double num                    = 0;
        sender auto sndr =
            just() | bulk(blocks, [&num](uint32_t idx) {
                const auto start = chunk_size * idx;
                for (auto j = start; j < start + chunk_size; ++j) {
                    num += std::sin(static_cast<double>(j));
                }
            });
        this_thread::sync_wait(sndr);
        println("result: {}", num);
    }

    {
        auto timer = set_timer("multi-thread");

        constexpr uint64_t count      = 1024UL * 1024 * 1024;
        constexpr auto parallelism    = 4;
        constexpr uint64_t chunk_size = count / parallelism;

        std::atomic<double> num = 0;
        thread_pool pool{ parallelism };
        scheduler auto sch = pool.get_scheduler();
        sender auto sndr =
            schedule(sch) | bulk(parallelism, [&num](uint32_t idx) {
                const auto start = idx * chunk_size;
                double local     = 0;
                for (auto j = start; j < start + chunk_size; ++j) {
                    local += std::sin(static_cast<double>(j));
                }
                num += local;
            });
        this_thread::sync_wait(sndr);
        println("result: {}", num.load());
    }

    return 0;
}
