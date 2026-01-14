// IWYU pragma: private, include <neutron/metafn.hpp>
#pragma once
#include "neutron/detail/metafn/append.hpp"
#include "neutron/detail/metafn/cat.hpp"
#include "neutron/detail/metafn/empty.hpp"
#include "neutron/detail/metafn/filt.hpp"
#include "neutron/detail/metafn/first_last.hpp"
#include "neutron/detail/metafn/specific.hpp"
#include "neutron/detail/metafn/substitute.hpp"

namespace neutron {

template <
    template <typename...> typename Template, typename Ty, typename Target>
struct insert_type_list_inplace {
    template <typename T>
    using _is_the_type_list = is_specific_type_list<Template, T>;
    using type              = std::remove_pointer_t<decltype([]() {
        using filted_type = type_list_filt_t<_is_the_type_list, Target>;
        if constexpr (is_empty_template<filted_type>::value) {
            using fixed_type =
                typename append_type_list<Template, Target>::type;
            using filted_type = type_list_filt_t<_is_the_type_list, fixed_type>;
            using current_type = type_list_first_t<filted_type>;
            using result       = type_list_substitute_t<
                                   fixed_type, current_type,
                                   type_list_cat_t<current_type, Template<Ty>>>;
            return static_cast<result*>(nullptr);
        } else {
            using current_type = type_list_first_t<filted_type>;
            using result       = type_list_substitute_t<
                                   Target, current_type,
                                   type_list_cat_t<current_type, Template<Ty>>>;
            return static_cast<result*>(nullptr);
        }
    }())>;
};
template <
    template <typename...> typename Template, typename Ty, typename Target>
using insert_type_list_inplace_t =
    typename insert_type_list_inplace<Template, Ty, Target>::type;

} // namespace neutron
