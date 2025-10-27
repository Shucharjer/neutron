#include <cstdint>
#include <type_traits>
#include <neutron/pipeline.hpp>
#include <neutron/type_list.hpp>

using namespace neutron;

// may need other design
template <typename>
struct app_require : std::false_type {};
template <typename App>
concept basic_app_could_do = requires(std::remove_cvref_t<App>& app) { app.run(); };
struct phony_app {
    void run();
};
template <basic_app_could_do App>
struct app_require<App> : std::true_type {};

template <typename... Lists>
class basic_application {
public:
    basic_application()                                    = default;
    basic_application(const basic_application&)            = delete;
    basic_application& operator=(const basic_application&) = delete;
    basic_application(basic_application&&)                 = delete;
    basic_application& operator=(basic_application&&)      = delete;
    ~basic_application()                                   = default;

    template <typename... AnotherLists>
    constexpr basic_application(basic_application<AnotherLists...>&& that) noexcept {} // NOLINT

    static basic_application create() { return {}; }
    void run() {}
};
using application = basic_application<>;

template <typename...>
struct system_list {};
enum class stage : uint8_t {
    startup,
    update,
    shutdown
};
template <stage Stage, auto System, typename... Requires>
struct add_system_fn : adaptor_closure<add_system_fn<Stage, System, Requires...>> {
    using input_require = app_require<void>;
    using output_type   = phony_app;

    template <typename App>
    constexpr auto operator()(App&& app) const {
        return insert_type_list_inplace_t<system_list, add_system_fn, App>{ std::forward<App>(
            app) };
    }
};

template <stage Stage, auto System, typename... Requires>
constexpr inline add_system_fn<Stage, System, Requires...> add_system;

void create_entities() {}
void echo_entities() {}
void apply_health() {}

template <auto... Systems>
struct after {};

int main() {
    using enum stage;
    auto myapp = application::create() | add_system<update, echo_entities> |
                 add_system<startup, create_entities> |
                 add_system<update, apply_health, after<echo_entities>>;

    auto pipeline = add_system<startup, create_entities> | add_system<update, echo_entities>;
    auto result   = application::create() | pipeline;
    // compile time system sort
    // run

    return 0;
}
