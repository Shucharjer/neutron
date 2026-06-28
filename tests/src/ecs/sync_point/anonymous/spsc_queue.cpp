#include <neutron/ecs.hpp>

using namespace neutron;
using enum stage;
using enum strategy;

void producer(sync_point<anonymous<"test">, strategy::spsc_queue, output<int>>) {
    //
}

void consumer(sync_point<anonymous<"test">, strategy::spsc_queue, input<int>>) {
    //
}

int main() {
    //

    return 0;
}

