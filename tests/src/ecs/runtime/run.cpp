#include <tuple>
#include <neutron/ecs.hpp>

using namespace neutron;

void test_run_empty() {
    struct app {
        static app create() noexcept { return {}; }
        void run() {}
    };

    app::create() | run_worlds<>();
}

void test_run_config() {
    struct app {
        static app create() noexcept { return {}; }
        void run(const std::tuple<int>& conf) {}
    };

    app::create() | run_worlds<>(32);
}

void test_run_raw_config() {
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
void test_run_world() { app::create() | run_worlds<world_desc>(); }

struct config_app {
    static app create() noexcept { return {}; }
    template <auto Worlds>
    void run(const std::tuple<int>& conf) {
        //
    }
};
void test_run_world_with_config() {
    config_app::create() | run_worlds<world_desc>();
}

int main() { return 0; }
