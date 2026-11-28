#pragma once
#include "neutron/detail/metafn/has.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT template <typename T, typename Tag>
concept type_list_with_tag = tagged_type_list_has_v<T, Tag>;

ATOM_EXPORT template <typename T, typename Tag>
concept value_list_with_tag = tagged_value_list_has_tag_v<T, Tag>;

ATOM_EXPORT template <typename T, typename Tag>
concept list_with_tag = tagged_list_has_tag_v<T, Tag>;

} // namespace neutron
