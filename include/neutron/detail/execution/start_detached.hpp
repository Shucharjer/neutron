// IWYU pragma: private, include <neutron/execution.hpp>
// compatible for beman::execution and standard
#pragma once
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>
#include "neutron/detail/execution/core.hpp" // IWYU pragma: keep
#include "neutron/detail/macros.hpp"
#include "neutron/memory.hpp"
#include "neutron/utility.hpp"

namespace neutron::execution {

inline constexpr struct start_detached_t {

    template <typename Env>
    struct state_base {

        constexpr explicit state_base(Env env) noexcept(
            std::is_nothrow_move_constructible_v<Env>)
            : base_env(std::move(env)) {}

        state_base(const state_base&)            = delete;
        state_base(state_base&&)                 = delete;
        state_base& operator=(const state_base&) = delete;
        state_base& operator=(state_base&&)      = delete;

        constexpr ~state_base() noexcept = default;

        constexpr void complete() noexcept { destroy(this); }

        ATOM_NO_UNIQUE_ADDR Env base_env;
        void (*destroy)(state_base*) noexcept = nullptr;
    };

    template <typename Env>
    struct receiver {
        using receiver_concept = receiver_tag;

        template <typename... Vals>
        constexpr void set_value(Vals&&...) noexcept {
            op->complete();
        }

        template <typename Error>
        [[noreturn]] void set_error(Error&&) noexcept {
            std::terminate();
        }

        constexpr void set_stopped() noexcept { op->complete(); }

        [[nodiscard]] constexpr auto get_env() const noexcept -> const Env& {
            return op->base_env;
        }

        state_base<Env>* op;
    };

    template <typename Sndr, typename Env>
    struct opstate : state_base<Env> {
        using op_t = connect_result_t<Sndr, receiver<Env>>;
        static_assert(std::is_nothrow_destructible_v<op_t>);
        op_t op;

        // NOLINTNEXTLINE
        constexpr opstate(Sndr&& sndr, Env env)
            : state_base<Env>(std::move(env)),
              op(connect(std::forward<Sndr>(sndr), receiver<Env>{ this })) {
            state_base<Env>::destroy = &opstate::destroy_self;
        }

        static constexpr void destroy_self(state_base<Env>* payload) noexcept {
            auto* self = static_cast<opstate*>(payload);

            auto alloc = [self] {
                if constexpr (requires { get_allocator(self->base_env); }) {
                    using alloc_ref_t =
                        std::remove_cvref_t<decltype(get_allocator(
                            self->base_env))>;
                    return rebind_alloc_t<alloc_ref_t, opstate>{ get_allocator(
                        self->base_env) };
                } else {
                    return std::allocator<opstate>{};
                }
            }();
            using alloc_t        = decltype(alloc);
            using alloc_traits_t = std::allocator_traits<alloc_t>;

            alloc_traits_t::destroy(alloc, self);
            alloc_traits_t::deallocate(alloc, self, 1);
        }
    };

    template <typename Sndr, typename Env = env<>>
    requires sender_to<Sndr, receiver<std::decay_t<Env>>>
    constexpr void apply_sender(Sndr&& sndr, Env&& env = {}) const {
        using opstate_t = opstate<Sndr, std::decay_t<Env>>;
        auto alloc      = [&env] {
            if constexpr (requires { get_allocator(env); }) {
                using alloc_ref_t =
                    std::remove_cvref_t<decltype(get_allocator(env))>;
                return rebind_alloc_t<alloc_ref_t, opstate_t>{ get_allocator(
                    env) };
            } else {
                return std::allocator<opstate_t>{};
            }
        }();
        using alloc_t        = decltype(alloc);
        using alloc_traits_t = std::allocator_traits<alloc_t>;
        opstate_t* op        = alloc_traits_t::allocate(alloc, 1);
        auto guard = make_exception_guard([alloc, op]() mutable noexcept {
            alloc_traits_t::deallocate(alloc, op, 1);
        });
        alloc_traits_t::construct(
            alloc, op, std::forward<Sndr>(sndr), std::forward<Env>(env));
        guard.dismiss();
        start(op->op);
    }

    template <sender Sndr>
    constexpr void operator()(Sndr&& sndr) const {
        auto dom = ::neutron::execution::get_domain(sndr);
        return ::neutron::execution::apply_sender(
            dom, *this, std::forward<Sndr>(sndr));
    }
} start_detached;

} // namespace neutron::execution
