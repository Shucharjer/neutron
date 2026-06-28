#pragma once
#include <atomic>

namespace neutron {

template <auto Next>
class intrusive_spsc_queue;

template <typename Node, Node* Node::* Next>
class intrusive_spsc_queue<Next> {
public:
    intrusive_spsc_queue() noexcept {
        (stub_.*Next).store(nullptr, std::memory_order_relaxed);
    }

    bool push_back(Node* node) noexcept {
        // (node->*Next).store(nullptr, std::memory_order_relaxed);
        // auto* prev = tail_.exchange(node, std::memory_order_acq_rel);
        // (prev->*Next).store(node, std::memory_order_release);
        // return prev == &stub_;
    }

    Node* pop_front() noexcept {
        // auto* head = head_;
        // auto* next = (head->*Next).load(std::memory_order_acquire);
        // if (head == &stub_) {
        //     if (next == nullptr) {
        //         return nullptr;
        //     }

        //     head_ =
        // }
    }

private:
    /// @brief A dummy node.
    /// It holds nullptr on just initiated.
    /// Then we store
    Node stub_;

    Node* head_{ &stub_ };

    Node* tail_{ &stub_ };
};

} // namespace neutron
