#pragma once
#include "./adaptor_closure.hpp"

namespace neutron {
/**
 * @brief Closure.
 *
 * It could be used as range adaptor closure.
 * @tparam Fn Function called in operator()
 * @tparam Args Arguments that
 */
template <typename Fn, typename... Args>
class closure : range_adaptor_closure<closure<Fn, Args...>> {
    using index_sequence = std::index_sequence_for<Args...>;
    using self_type      = closure;

public:
    using pipeline_tag = void;

    static_assert((std::same_as<std::decay_t<Args>, Args> && ...));
    static_assert(std::is_empty_v<Fn> || std::is_default_constructible_v<Fn>);

    explicit constexpr closure(auto&&... args) noexcept(
        std::conjunction_v<std::is_nothrow_constructible<
            std::tuple<Args...>, decltype(args)...>>)
    requires std::is_constructible_v<std::tuple<Args...>, decltype(args)...>
        : args_(std::make_tuple(std::forward<decltype(args)>(args)...)) {}

    constexpr decltype(auto) operator()(auto&& arg) & noexcept(noexcept(
        _call(*this, std::forward<decltype(arg)>(arg), index_sequence{})))
    requires std::invocable<Fn, decltype(arg), Args&...>
    {
        return _call(*this, std::forward<decltype(arg)>(arg), index_sequence{});
    }

    constexpr decltype(auto) operator()(auto&& arg) const& noexcept(noexcept(
        _call(*this, std::forward<decltype(arg)>(arg), index_sequence{})))
    requires std::invocable<Fn, decltype(arg), const Args&...>
    {
        return _call(*this, std::forward<decltype(arg)>(arg), index_sequence{});
    }

    template <typename Ty>
    requires std::invocable<Fn, Ty, Args...>
    constexpr decltype(auto) operator()(Ty&& arg) && noexcept(
        noexcept(_call(*this, std::forward<Ty>(arg), index_sequence{}))) {
        return _call(*this, std::forward<Ty>(arg), index_sequence{});
    }

    template <typename Ty>
    requires std::invocable<Fn, Ty, const Args...>
    constexpr decltype(auto) operator()(Ty&& arg) const&& noexcept(
        noexcept(_call(*this, std::forward<Ty>(arg), index_sequence{}))) {
        return _call(*this, std::forward<Ty>(arg), index_sequence{});
    }

private:
    template <typename SelfTy, typename Ty, size_t... Is>
    constexpr static decltype(auto)
        _call(SelfTy&& self, Ty&& arg, std::index_sequence<Is...>) noexcept(
            noexcept(Fn{}(
                std::forward<Ty>(arg),
                std::get<Is>(std::forward<SelfTy>(self).args_)...))) {
        static_assert(std::same_as<std::index_sequence<Is...>, index_sequence>);
        return Fn{}(
            std::forward<Ty>(arg),
            std::get<Is>(std::forward<SelfTy>(self).args_)...);
    }

    std::tuple<Args...> args_;
};

/**
 * @brief Making a closure without caring about the parameter types.
 *
 * @tparam Fn Closure function type.
 * @tparam Args Arguments types would be called in the closure. They could be
 * deduced.
 * @param args Arguments.
 * @return closure<Fn, std::decay_t<Args>...> Closure.
 */
template <typename Fn, typename... Args>
constexpr auto make_closure(Args&&... args)
    -> closure<Fn, std::decay_t<Args>...> {
    return closure<Fn, std::decay_t<Args>...>(std::forward<Args>(args)...);
}

} // namespace neutron
