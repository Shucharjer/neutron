#include <tuple>
#include <neutron/ecs.hpp>
#include "require.hpp"

using namespace neutron;

void test_run_without_worlds() {
    struct app {
        static app create() noexcept { return {}; }
        void run() {}
    };

    app::create() | run_worlds<>();
}

void test_run_without_worlds_forwards_tuple_config() {
    struct app {
        static app create() noexcept { return {}; }
        void run(const std::tuple<int>& conf) {}
    };

    app::create() | run_worlds<>(32);
}

void test_run_without_worlds_forwards_raw_config() {
    struct app {
        static app create() noexcept { return {}; }
        void run(int conf) {}
    };

    app::create() | run_worlds<>(32);
}

struct app {
    static app create() noexcept { return {}; }
    template <auto Worlds>
    void run() {}
};
void test_run_single_world() { app::create() | run_worlds<world_desc>(); }

struct config_app {
    static app create() noexcept { return {}; }
    template <auto Worlds>
    void run(const std::tuple<int>& conf) {
        //
    }
};
void test_run_single_world_with_config_app() {
    config_app::create() | run_worlds<world_desc>();
}

int main() {
    test_run_without_worlds();
    test_run_without_worlds_forwards_tuple_config();
    test_run_without_worlds_forwards_raw_config();
    test_run_single_world();
    test_run_single_world_with_config_app();
    return neutron_test::result();
}
