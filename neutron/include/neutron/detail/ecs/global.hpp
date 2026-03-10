#pragma once
#include "neutron/detail/metafn/definition.hpp"
#include "neutron/detail/metafn/filt.hpp"
#include "neutron/detail/tuple/shared_tuple.hpp"

namespace neutron {

/**
 * @brief The data could be accessed in every world.
 *
 * @tparam The requested global data.
 */
template <typename... Args>
struct global : std::tuple<Args&...> {
public:
    using const_list = type_list_filt_t<std::is_const, type_list<Args...>>;
    using const_list
};

} // namespace neutron
