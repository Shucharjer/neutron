#pragma once

namespace neutron {

template <typename Iter>
concept input_iterator = requires(const Iter iter) { *iter; };

template <typename Iter>
concept output_iterator = requires(Iter iter) { *iter; };

template <typename Iter>
concept forward_iterator = input_iterator<Iter> && requires(Iter iter) {
    ++iter;
    iter++;
};

template <typename Iter>
concept bidirectional_iterator = forward_iterator<Iter> && requires(Iter iter) {
    --iter;
    iter--;
};

template <typename Iter>
concept random_access_iterator =
    bidirectional_iterator<Iter> &&
    requires(Iter iter) { iter += static_cast<unsigned>(-1); };

} // namespace neutron
