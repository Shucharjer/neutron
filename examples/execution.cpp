// Compose senders and block until they complete.
#include <utility>
#include <neutron/execution.hpp>
#include <neutron/print.hpp>

using namespace neutron;
using namespace neutron::execution;

int main() {
    auto answer  = just(40) | then([](int value) {
                      return value + 2;
                  });
    auto [value] = this_thread::sync_wait(std::move(answer)).value();
    println("answer: {}", value);

    // when_all concatenates the values of all its children.
    auto both            = when_all(just(1), just(2.5));
    auto [integer, real] = this_thread::sync_wait(std::move(both)).value();
    println("when_all: {}, {}", integer, real);
}
