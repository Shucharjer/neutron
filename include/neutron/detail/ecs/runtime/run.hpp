// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <tuple>
#include <type_traits>
#include <utility>

namespace neutron {

template <typename Tuple, auto... Worlds>
class run_worlds_fn;

template <>
class run_worlds_fn<std::tuple<>> {
public:
    constexpr run_worlds_fn() noexcept = default;

    template <typename App>
    constexpr decltype(auto) operator()(App&& app) const
    requires requires { std::forward<App>(app).run(); }
    {
        return std::forward<App>(app).run();
    }
};

template <auto... Worlds>
class run_worlds_fn<std::tuple<>, Worlds...> {
public:
    constexpr run_worlds_fn() noexcept = default;

    template <typename App>
    constexpr decltype(auto) operator()(App&& app) const
    requires requires { std::forward<App>(app).template run<Worlds...>(); }
    {
        return std::forward<App>(app).template run<Worlds...>();
    }
};

template <typename... Args>
class run_worlds_fn<std::tuple<Args...>> {
    using tuple_type = std::tuple<Args...>;
    std::tuple<Args...> tup_;

public:
    template <typename... ArgTys>
    constexpr run_worlds_fn(ArgTys&&... args) noexcept(
        std::is_nothrow_constructible_v<tuple_type, ArgTys...>)
        : tup_(std::forward<ArgTys>(args)...) {}

    template <typename App>
    constexpr auto operator()(App&& app) const&
    requires requires { std::forward<App>(app).run(tup_); }
    {
        return std::forward<App>(app).run(tup_);
    }

    template <typename App>
    constexpr auto operator()(App&& app) const&
    requires(
        !requires { std::forward<App>(app).run(tup_); } &&
        requires(Args&... args) { std::forward<App>(app).run(args...); })
    {
        return std::apply(
            [&app](auto&&... args) {
                return std::forward<App>(app).run(args...);
            },
            tup_);
    }

    template <typename App>
    constexpr auto operator()(App&& app) &&
    requires requires { std::forward<App>(app).run(std::move(tup_)); }
    {
        return std::forward<App>(app).run(std::move(tup_));
    }

    template <typename App>
    constexpr auto operator()(App&& app) &&
    requires(
        !requires { std::forward<App>(app).run(std::move(tup_)); } &&
        requires(Args&... args) {
            std::forward<App>(app).run(std::move(args)...);
        })
    {
        return std::apply(
            [&app](auto&&... args) {
                return std::forward<App>(app).run(std::move(args)...);
            },
            tup_);
    }
};

template <typename... Args, auto... Worlds>
class run_worlds_fn<std::tuple<Args...>, Worlds...> {
    using tuple_type = std::tuple<Args...>;
    std::tuple<Args...> tup_;

public:
    template <typename... ArgTys>
    constexpr run_worlds_fn(ArgTys&&... args) noexcept(
        std::is_nothrow_constructible_v<tuple_type, ArgTys...>)
        : tup_(std::forward<ArgTys>(args)...) {}

    template <typename App>
    constexpr auto operator()(App&& app) const&
    requires requires { std::forward<App>(app).template run<Worlds...>(tup_); }
    {
        return std::forward<App>(app).template run<Worlds...>(tup_);
    }

    template <typename App>
    constexpr auto operator()(App&& app) const&
    requires(
        !requires { std::forward<App>(app).template run<Worlds...>(tup_); } &&
        requires(Args&... args) {
            std::forward<App>(app).template run<Worlds...>(args...);
        })
    {
        return std::apply(
            [&app](auto&&... args) {
                return std::forward<App>(app).template run<Worlds...>(args...);
            },
            tup_);
    }

    template <typename App>
    constexpr auto operator()(App&& app) &&
    requires requires {
        std::forward<App>(app).template run<Worlds...>(std::move(tup_));
    }
    {
        return std::forward<App>(app).template run<Worlds...>(std::move(tup_));
    }

    template <typename App>
    constexpr auto operator()(App&& app) &&
    requires(
        !requires {
            std::forward<App>(app).template run<Worlds...>(std::move(tup_));
        } &&
        requires(Args&... args) {
            std::forward<App>(app).template run<Worlds...>(std::move(args)...);
        })
    {
        return std::apply(
            [&app](auto&&... args) {
                std::forward<App>(app).template run<Worlds...>(
                    std::move(args)...);
            },
            tup_);
    }
};

template <typename App, typename Tup, auto... Worlds>
constexpr decltype(auto)
    operator|(App&& app, const run_worlds_fn<Tup, Worlds...>& fn) {
    return fn(std::forward<App>(app));
}

template <typename App, typename Tup, auto... Worlds>
constexpr decltype(auto)
    operator|(App&& app, run_worlds_fn<Tup, Worlds...>&& fn) {
    return std::move(fn)(std::forward<App>(app));
}

template <auto... Worlds>
struct _run_worlds_wrapper {
    template <typename... Args>
    constexpr auto operator()(Args&&... args) const
        noexcept(std::is_nothrow_constructible_v<
                 std::tuple<std::decay_t<Args>...>, Args...>) {
        return run_worlds_fn<std::tuple<std::decay_t<Args>...>, Worlds...>(
            std::forward<Args>(args)...);
    }
};

template <auto... Worlds>
constexpr inline _run_worlds_wrapper<Worlds...> run_worlds;

} // namespace neutron
