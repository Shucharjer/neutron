#pragma once
#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include "neutron/detail/concepts/movable_value.hpp"
#include "neutron/detail/execution/connect.hpp"
#include "neutron/detail/execution/fwd.hpp"
#include "neutron/detail/execution/fwd_env.hpp"
#include "neutron/detail/execution/product_type.hpp"
#include "neutron/detail/execution/set_error.hpp"
#include "neutron/detail/execution/set_stopped.hpp"
#include "neutron/detail/execution/set_value.hpp"
#include "neutron/detail/metafn/size.hpp"
#include "neutron/detail/utility/forward_like.hpp"
#include "neutron/detail/utility/get.hpp"

namespace neutron::execution {

template <typename Tag>
concept _completion_tag = one_of<Tag, set_value_t, set_error_t, set_stopped_t>;

template <template <typename...> typename T, typename... Args>
concept _valid_specialization = requires { typename T<Args...>; };

struct _default_impls {
    static constexpr auto get_attrs =
        [](const auto&, const auto&... child) noexcept -> decltype(auto) {
        if constexpr (sizeof...(child) == 1) {
            return (_fwd_env(::neutron::execution::get_env(child)), ...);
        } else {
            return empty_env{};
        }
    };

    static constexpr auto get_env =
        [](auto, auto&, const auto& rcvr) noexcept -> decltype(auto) {
        return _fwd_env(::neutron::execution::get_env(rcvr));
    };

    static constexpr auto get_state =
        []<class Sndr, class Rcvr>(
            Sndr&& sndr, Rcvr& rcvr) noexcept -> decltype(auto) {
        return std::apply(
            [](auto&&, auto&& data, auto&&...) {
                return forward_like<Sndr>(data);
            },
            sndr);
    };

    static constexpr auto start = [](auto&, auto&,
                                     auto&... ops) noexcept -> void {
        (execution::start(ops), ...);
    };

    static constexpr auto complete =
        []<class Index, class Rcvr, class Tag, class... Args>(
            Index, auto& state, Rcvr& rcvr, Tag,
            Args&&... args) noexcept -> void
    requires std::is_invocable_v<Tag, Rcvr, Args...>
    {
        static_assert(Index::value == 0);
        Tag()(std::move(rcvr), std::forward<Args>(args)...);
    };
};

template <typename Tag>
struct _impls_for : _default_impls {};

template <typename Sndr, typename Rcvr>
using _state_type = std::decay_t<std::invoke_result_t<
    decltype(_impls_for<tag_of_t<Sndr>>::get_state), Sndr, Rcvr&>>;

template <typename Index, typename Sndr, typename Rcvr>
using _env_type = std::invoke_result_t<
    decltype(_impls_for<tag_of_t<Sndr>>::get_env), Index,
    _state_type<Sndr, Rcvr>&, const Rcvr&>;

template <typename Sndr, size_t I = 0>
using _child_type = decltype(get<I + 2>(std::declval<Sndr>()));

template <typename Sndr>
using _indices_for = std::remove_reference_t<Sndr>::_indices_for;

template <typename Sndr, typename Rcvr>
struct _basic_state {
    using _tag_t = tag_of_t<Sndr>;
    constexpr _basic_state(Sndr&& sndr, Rcvr&& rcvr)
        : rcvr(std::move(rcvr)),
          state(
              _impls_for<_tag_t>::get_state(
                  std::forward<Sndr>(sndr), this->rcvr)) {}

    Rcvr rcvr;
    _state_type<Sndr, Rcvr> state;
};
template <typename Sndr, typename Rcvr>
_basic_state(Sndr&&, Rcvr&&) -> _basic_state<Sndr&&, Rcvr>;

template <typename Sndr, typename Rcvr, typename Index>
requires _valid_specialization<_env_type, Index, Sndr, Rcvr>
struct _basic_receiver {
    using receiver_concept = receiver_t;

    using _tag_t                          = tag_of_t<Sndr>;
    using _state_t                        = _state_type<Sndr, Rcvr>;
    static constexpr const auto& complete = _impls_for<_tag_t>::complete;

    template <typename... Args>
    requires std::is_invocable_v<
        decltype(complete), Index, _state_t&, Rcvr&, set_value_t, Args...>
    void set_value(Args&&... args) && noexcept {
        complete(
            Index(), op->state, op->rcvr, set_value_t(),
            std::forward<Args>(args)...);
    }

    template <typename Error>
    requires std::is_invocable_v<
        decltype(complete), Index, _state_t&, Rcvr&, set_error_t, Error>
    void set_error(Error&& err) && noexcept {
        complete(
            Index(), op->state, op->rcvr, set_error_t(),
            std::forward<Error>(err));
    }

    void set_stopped() && noexcept {
        complete(Index(), op->state, op->rcvr, set_stopped_t());
    }

    auto get_env() const noexcept -> _env_type<Index, Sndr, Rcvr> {
        return _impls_for<_tag_t>::get_env(Index(), op->state, op->rcvr);
    }

    _basic_state<Sndr, Rcvr>* op;
};

inline constexpr struct connect_all_t {
    template <sender Sndr, receiver Rcvr, typename S, size_t... Is>
    constexpr auto operator()(
        _basic_state<Sndr, Rcvr>* op, S&& sndr,
        std::index_sequence<Is...>) const noexcept( // clang-format off
            noexcept(std::apply([op](auto&& tag, auto&& data, auto&&... child) {
                return _product_type{ connect(
                    forward_like<Sndr>(child)...,
                    _basic_receiver<
                        Sndr,
                        Rcvr,
                        std::integral_constant<std::size_t, Is>>{ op })... };
            },
            std::forward<decltype(sndr)>(sndr))))
        // clang-format on
        -> decltype(auto) {
        return std::apply(
            [op](auto&& tag, auto&& data, auto&&... child) {
                return _product_type{ connect(
                    forward_like<Sndr>(child)...,
                    _basic_receiver<
                        Sndr, Rcvr, std::integral_constant<std::size_t, Is>>{
                        op })... };
            },
            std::forward<decltype(sndr)>(sndr));
    };
} connect_all;

template <typename Sndr, typename Rcvr>
using _connect_all_result = decltype(connect_all(
    std::declval<_basic_state<Sndr, Rcvr>*>(), std::declval<Sndr>(),
    std::declval<_indices_for<Sndr>>()));

template <typename Sndr, typename Rcvr>
requires _valid_specialization<_state_type, Sndr, Rcvr> &&
         _valid_specialization<_connect_all_result, Sndr, Rcvr>
struct _basic_operation : public _basic_state<Sndr, Rcvr> {
    using operation_state_concept = operation_state_t;
    using _tag_t                  = tag_of_t<Sndr>;
    using _inner_ops_t            = _connect_all_result<Sndr, Rcvr>;
    _inner_ops_t inner_ops;

    constexpr _basic_operation(Sndr&& sndr, Rcvr&& rcvr)
        : _basic_state<Sndr, Rcvr>(std::forward<Sndr>(sndr), std::move(rcvr)),
          inner_ops(connect_all(
              this, std::forward<Sndr>(sndr), _indices_for<Sndr>())) {}

    void start() & noexcept {
        [this]<size_t... Is>(std::index_sequence<Is...>) {
            _impls_for<_tag_t>::start(
                this->state, this->rcvr, get<Is>(inner_ops)...);
        }(std::make_index_sequence<type_list_size_v<_inner_ops_t>>());
    }
};
template <typename Sndr, typename Rcvr>
_basic_operation(Sndr&&, Rcvr&&) -> _basic_operation<Sndr&&, Rcvr>;

template <typename Sndr, typename Env>
struct _completion_signatures_for_impl {
    static_assert(
        false, "no suitable specification for 'completion-signatures-for'");
};

template <typename Sndr, typename Env>
using _completion_signatures_for = typename _completion_signatures_for_impl<
    std::remove_cvref_t<Sndr>, Env>::type;

template <typename Tag, typename Data, typename... Child>
struct _basic_sender : public _product_type<Tag, Data, Child...> {
    using sender_concept = sender_t;
    using _indices_for   = std::index_sequence_for<Child...>;

    using _product_type<Tag, Data, Child...>::_product_type;

    decltype(auto) get_env() const noexcept {
        return std::apply(
            [](auto&&, auto&& data, auto&&... child) {
                return _impls_for<Tag>::get_attrs(data, child...);
            },
            *this);
    }

    template <typename NotRcvr>
    requires(!receiver<NotRcvr>)
    auto connect(NotRcvr) {
        static_assert(
            false,
            "the parameter type do not satisfy the concept of 'receiver'");
    }

    template <receiver Rcvr>
    auto connect(Rcvr rcvr) & -> _basic_operation<_basic_sender&, Rcvr> {
        return { *this, std::move(rcvr) };
    }

    template <receiver Rcvr>
    auto connect(Rcvr rcvr) && -> _basic_operation<_basic_sender&&, Rcvr> {
        return { std::move(*this), std::move(rcvr) };
    }

    template <receiver Rcvr>
    auto connect(
        Rcvr rcvr) const& -> _basic_operation<const _basic_sender&, Rcvr> {
        return { *this, std::move(rcvr) };
    }

    template <receiver Rcvr>
    auto connect(
        Rcvr rcvr) const&& -> _basic_operation<const _basic_sender&&, Rcvr> {
        return { std::move(*this), std::move(rcvr) };
    }

    template <class Env>
    auto get_completion_signatures(Env&&) & noexcept
        -> _completion_signatures_for<_basic_sender, Env> {
        return {};
    }

    template <class Env>
    auto get_completion_signatures(Env&&) && noexcept
        -> _completion_signatures_for<_basic_sender, Env> {
        return {};
    }

    template <class Env>
    auto get_completion_signatures(Env&&) const& noexcept
        -> _completion_signatures_for<const _basic_sender, Env> {
        return {};
    }

    template <class Env>
    auto get_completion_signatures(Env&&) const&& noexcept
        -> _completion_signatures_for<const _basic_sender, Env> {
        return {};
    }

private:
    template <_decays_to<_basic_sender> Self, receiver Rcvr>
    static auto _connect_impl(Self&& self, Rcvr rcvr);
};

template <std::semiregular Tag, movable_value Data, typename... Child>
requires(sender<Child> && ...)
constexpr decltype(auto) make_sender(Tag tag, Data&& data, Child&&... child) {
    return _basic_sender<Tag, std::decay_t<Data>, std::decay_t<Child>...>(
        tag, std::forward<Data>(data), std::forward<Child>(child)...);
}

} // namespace neutron::execution

namespace std {

template <typename Tag, typename Data, typename... Child>
struct tuple_size<::neutron::execution::_basic_sender<Tag, Data, Child...>> :
    std::tuple_size<neutron::execution::_product_type<Tag, Data, Child...>> {};

template <size_t Index, typename Tag, typename Data, typename... Child>
struct tuple_element<
    Index, ::neutron::execution::_basic_sender<Tag, Data, Child...>> :
    std::tuple_element<
        Index, neutron::execution::_product_type<Tag, Data, Child...>> {};

} // namespace std
