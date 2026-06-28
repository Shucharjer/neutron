#pragma once
#include <atomic>

namespace neutron::internal {

template <typename T>
struct _lockfree_node {
    T value;
    _lockfree_node* next;
};

template <typename T>
struct _lockfree_atomic_node {
    T value;
    std::atomic<_lockfree_atomic_node*> next;
};

} // namespace neutron::internal
