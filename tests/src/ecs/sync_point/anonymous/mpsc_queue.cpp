#include <neutron/ecs.hpp>

using namespace neutron;
using enum stage;
using enum strategy;

void producer(sync_point<multi, mpsc_queue, output<>>) {
    //
}

void consumer(sync_point<multi, mpsc_queue, input<>>) {
    //
}

int main() {
    //

    return 0;
}
