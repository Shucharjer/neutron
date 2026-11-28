// IWYU pragma: private, include <neutron/functional.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <memory>
#endif
#include "neutron/detail/functional/delegate.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

struct _sink_base {
    void (*destroy)(_sink_base*);
};

ATOM_EXPORT template <typename Event, typename Alloc = std::allocator<Event>>
class sink : public _sink_base {
public:
    sink()
        : _sink_base{ .destroy = [](_sink_base* payload) {
              static_cast<sink*>(payload)->~sink();
          } } {}

private:
};

} // namespace neutron
