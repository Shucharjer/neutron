#include <neutron/ecs.hpp>
#include <neutron/print.hpp>

using namespace neutron;
using enum stage;
using enum strategy;

void producer(sync_point<multi, strategy::spsc_queue, output<int>> sync_point) {
    // auto [output] = sync_point;
    // auto [iqueue] = output;
    // while (true) {
    //     iqueue.push(42);
    // }
}

void consumer(sync_point<multi, strategy::spsc_queue, input<int>> sync_point) {
    // auto [input]  = sync_point;
    // auto [iqueue] = input;
    // while (!iqueue.empty()) {
    //     auto ans = iqueue.pop();
    //     ::println("the answer is {}", ans);
    // }
}

int main() {
    //

    return 0;
}
