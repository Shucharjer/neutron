#include <latch>
#include <neutron/print.hpp>
#include <neutron/thread_pool.hpp>

using namespace neutron;

int main() {
    thread_pool thread_pool;

    // enqueue & latch test
    const auto task_num = 1000000;

    std::latch latch(task_num);
    for (auto i = 0; i < task_num; ++i) {
        auto fn = []() {};
        thread_pool.submit(fn);
    }

    latch.wait();
    print("all tasks are finished");

    return 0;
}
