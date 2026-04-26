#pragma once
#include "neutron/detail/concepts/unstoppable_token.hpp"
#include "neutron/detail/execution/fwd.hpp"

namespace neutron::execution {

template <typename Sch, typename Env>
concept _infallible_scheduler =
    scheduler<Sch> &&
    (std::same_as<
         completion_signatures<set_value_t()>,
         completion_signatures_of_t<decltype(schedule(declval<Sch>())), Env>> ||
     (!unstoppable_token<stop_token_of_t<Env>> &&
      (std::same_as<
           completion_signatures<set_value_t(), set_stopped_t()>,
           completion_signatures_of_t<
               decltype(schedule(declval<Sch>())), Env>> ||
       std::same_as<
           completion_signatures<set_stopped_t(), set_value_t()>,
           completion_signatures_of_t<
               decltype(schedule(declval<Sch>())), Env>>)));

}
