#pragma once

#include <cstddef>
#include <exception>
#include <optional>
#include "neutron/detail/execution/fwd.hpp"
namespace neutron::execution::parallel_scheduler_replacement {

struct receiver_proxy {
protected:
    virtual bool query_env(/* unspecified */) noexcept = 0; // exposition only

public:
    virtual void set_value() noexcept                   = 0;
    virtual void set_error(std::exception_ptr) noexcept = 0;
    virtual void set_stopped() noexcept                 = 0;

    template <typename P, _class_type Query>
    std::optional<P> try_query(Query q) const noexcept {
        //
    }
};

struct bulk_item_receiver_proxy : receiver_proxy {
    virtual void execute(std::size_t, std::size_t) noexcept = 0;
};

} // namespace neutron::execution::parallel_scheduler_replacement
