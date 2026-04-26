#pragma once

#include <cstddef>
#include <span>
#include "neutron/detail/execution/parallel_scheduler_replacement/receiver_proxy.hpp"
namespace neutron::execution::parallel_scheduler_replacement {

struct parallel_scheduler_backend {
    virtual ~parallel_scheduler_backend() = default;

    virtual void schedule(receiver_proxy&, std::span<std::byte>) noexcept = 0;
    virtual void schedule_bulk_chunked(
        std::size_t, bulk_item_receiver_proxy&,
        std::span<std::byte>) noexcept = 0;
    virtual void schedule_bulk_unchunked(
        std::size_t, bulk_item_receiver_proxy&,
        std::span<std::byte>) noexcept = 0;
};

} // namespace neutron::execution::parallel_scheduler_replacement
