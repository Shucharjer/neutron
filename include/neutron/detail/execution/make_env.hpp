#pragma once
#include <type_traits>
#include <utility>

namespace neutron::execution {

template <typename Query, typename Value>
class make_env {
public:
    template <typename Val>
    constexpr make_env(const Query& query, Val&& val)
        : val_(std::forward<Val>(val)) {}

    constexpr auto query(const Query&) const noexcept -> const Value& {
        return val_;
    }

    constexpr auto query(const Query&) noexcept -> Value& { return val_; }

private:
    Value val_;
};

template <typename Query, typename Val>
make_env(Query&&, Val&&)
    -> make_env<std::remove_cvref_t<Query>, std::remove_cvref_t<Val>>;

} // namespace neutron::execution
