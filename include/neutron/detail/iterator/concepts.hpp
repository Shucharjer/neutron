// IWYU pragma, include <neutron/iterator.hpp>
#pragma once
#include <iterator>

namespace neutron {

using ::std::input_iterator;
using ::std::forward_iterator;
using ::std::random_access_iterator;
using ::std::contiguous_iterator;

template <typename It>
concept nothrow_forward_iterator =
    forward_iterator<It> && requires(It& it, It another) {
        { it++ } noexcept;
        { it = another } noexcept;
        { it == another } noexcept;
        { it != another } noexcept;
        { *it } noexcept;
    };

} // namespace neutron
