// IWYU pragma, include <neutron/iterator.hpp>
#pragma once
#ifndef ATOM_BUILDING_MODULE
    #include <iterator>
#endif
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT using ::std::input_iterator;
ATOM_EXPORT using ::std::forward_iterator;
ATOM_EXPORT using ::std::random_access_iterator;
ATOM_EXPORT using ::std::contiguous_iterator;

ATOM_EXPORT template <typename It>
concept nothrow_forward_iterator =
    forward_iterator<It> && requires(It& it, It another) {
        { it++ } noexcept;
        { it = another } noexcept;
        { it == another } noexcept;
        { it != another } noexcept;
        { *it } noexcept;
    };

} // namespace neutron
