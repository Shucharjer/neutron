#pragma once
#include <cassert>
#include <utility>
#include "neutron/detail/macros.hpp"

namespace neutron {

template <auto Next>
class intrusive_queue;

template <typename Node, Node* Node::* Next>
class intrusive_queue<Next> {
public:
    constexpr intrusive_queue() noexcept = default;

    constexpr intrusive_queue(intrusive_queue&& that) noexcept
        : head_(std::exchange(that.head_, nullptr)),
          tail_(std::exchange(that.tail_, nullptr)) {}

    constexpr intrusive_queue& operator=(intrusive_queue&& that) noexcept {
        std::swap(head_, that.head_);
        std::swap(tail_, that.tail_);
        return *this;
    }

    constexpr intrusive_queue& operator=(intrusive_queue that) noexcept {
        std::swap(head_, that.head_);
        std::swap(tail_, that.tail_);
        return *this;
    }

    constexpr ~intrusive_queue() noexcept { assert(empty()); }

    ATOM_NODISCARD constexpr bool empty() const noexcept {
        return head_ == tail_;
    }

private:
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
};

} // namespace neutron
