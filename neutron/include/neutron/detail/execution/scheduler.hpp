#pragma once
#include "./fwd.hpp"

#include <concepts>
#include <type_traits>
#include "../tag_invoke.hpp"
#include "./queries.hpp"
#include "./sender.hpp"

namespace neutron::execution {

template <typename Sch>
concept _enable_scheduler = std::derived_from<
    typename std::remove_cvref_t<Sch>::scheduler_concept, scheduler_t>;

template <typename Sch>
concept _has_member_schedule = requires(std::remove_cvref_t<Sch> sch) {
    { sch.schedule() } -> sender;
};

constexpr inline struct schedule_t {
    template <typename Sch>
    requires _has_member_schedule<Sch>
    constexpr decltype(auto) operator()(Sch&& scheduler) const
        noexcept(noexcept(std::forward<Sch>(scheduler).schedule())) {
        return std::forward<Sch>(scheduler).schedule();
    }

    template <typename Sch>
    requires(!_has_member_schedule<Sch>) && tag_invocable<schedule_t, Sch>
    constexpr decltype(auto) operator()(Sch&& scheduler) const
        noexcept(nothrow_tag_invocable<schedule_t, Sch>) {
        return tag_invoke(schedule_t{}, std::forward<Sch>(scheduler));
    }
} schedule;

template <typename Sch>
concept _has_schedule = requires(Sch&& sch) {
    { schedule(std::forward<Sch>(sch)) } -> sender;
};

template <typename Ty>
constexpr auto _decay_copy(Ty) -> Ty;

template <typename Sch>
concept _sender_has_completion_scheduler = requires(Sch&& sch) {
    {
        _decay_copy(
            get_completion_scheduler<set_value_t>(
                get_env(schedule(std::forward<Sch>(sch)))))
    } -> std::same_as<std::remove_cvref_t<Sch>>;
};

template <typename Sch>
concept scheduler =
    _enable_scheduler<Sch> && queryable<Sch> && _has_schedule<Sch> &&
    _sender_has_completion_scheduler<Sch> &&
    std::equality_comparable<std::remove_cvref_t<Sch>> &&
    std::copy_constructible<Sch>;

} // namespace neutron::execution
