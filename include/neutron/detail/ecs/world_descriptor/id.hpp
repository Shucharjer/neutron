#pragma once
#include "neutron/detail/ecs/world_descriptor/fwd.hpp"
#include "neutron/detail/metafn/insert_inplace.hpp"
#include "neutron/tstring.hpp"

namespace neutron {

template <tstring Id>
struct _id : public descriptor_adaptor_closure<_id<Id>> {
    template <world_descriptor Descriptor>
    consteval auto operator()(Descriptor) const noexcept {
        return insert_tagged_type_list_inplace_t<
            _id<Id>, Descriptor, _id<Id>>{};
    }
};

template <tstring Id>
inline constexpr _id<Id> id;

} // namespace neutron
