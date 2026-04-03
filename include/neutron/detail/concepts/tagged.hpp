#pragma once
#include "neutron/detail/metafn/has.hpp"

namespace neutron {

template <typename T, typename Tag>
concept type_list_with_tag = tagged_type_list_has_v<T, Tag>;

template <typename T, typename Tag>
concept value_list_with_tag = tagged_value_list_has_tag_v<T, Tag>;

template <typename T, typename Tag>
concept list_with_tag = tagged_list_has_tag_v<T, Tag>;

}
