#pragma once
#include <type_traits>
#include <utility>
#include "neutron/detail/execution/core.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron::internal {

template <execution::sender Sender>
struct conditional_sender {
    using sender_concept = execution::sender_tag;

    bool enabled;
    Sender sender;

    // stdexec queries completion signatures through a member function that
    // takes the environment as an argument.
    template <typename Env>
    auto get_completion_signatures(Env&&) const noexcept
        -> execution::completion_signatures_of_t<Sender, Env> {
        return {};
    }

    // beman queries them through a static member template evaluated as a
    // constant expression; both spellings can coexist.
    template <typename Self, typename... Env>
    static consteval auto get_completion_signatures() noexcept
        -> execution::completion_signatures_of_t<Sender, Env...> {
        return {};
    }

    template <typename Receiver>
    struct operation {
        using operation_state_concept = execution::operation_state_tag;

        struct receiver {
            using receiver_concept = execution::receiver_tag;
            operation* owner;

            decltype(auto) get_env() const noexcept {
                return execution::get_env(owner->receiver_);
            }

            void set_value() && noexcept {
                execution::set_value(std::move(owner->receiver_));
            }

            template <typename Error>
            void set_error(Error&& error) && noexcept {
                execution::set_error(
                    std::move(owner->receiver_), std::forward<Error>(error));
            }

            void set_stopped() && noexcept {
                execution::set_stopped(std::move(owner->receiver_));
            }
        };

        bool enabled_;
        ATOM_NO_UNIQUE_ADDR Receiver receiver_;
        execution::connect_result_t<Sender, receiver> inner_;

        operation(bool enabled, Sender&& sender, Receiver receiver)
            : enabled_(enabled), receiver_(std::move(receiver)),
              inner_(
                  execution::connect(
                      std::move(sender),
                      typename operation::receiver{ this })) {}

        operation(const operation&)            = delete;
        operation(operation&&)                 = delete;
        operation& operator=(const operation&) = delete;
        operation& operator=(operation&&)      = delete;

        void start() & noexcept {
            if (enabled_) {
                execution::start(inner_);
            } else {
                execution::set_value(std::move(receiver_));
            }
        }
    };

    template <execution::receiver Receiver>
    auto connect(Receiver receiver) && {
        return operation<Receiver>{ enabled, std::move(sender),
                                    std::move(receiver) };
    }
};

} // namespace neutron::internal
