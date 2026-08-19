// IWYU pragma: private, include <neutron/functional.hpp>
#pragma once
#include <memory>
#include "neutron/detail/functional/delegate.hpp"

namespace neutron {

struct _sink_base {
    void (*destroy)(_sink_base*);
};

template <typename Event, typename Alloc = std::allocator<Event>>
class sink : public _sink_base {
public:
    sink()
        : _sink_base{ .destroy = [](_sink_base* payload) {
              static_cast<sink*>(payload)->~sink();
          } } {}

private:
};

} // namespace neutron
