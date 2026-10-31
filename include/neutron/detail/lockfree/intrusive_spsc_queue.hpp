#pragma once
#include <atomic>
#include "neutron/detail/lockfree/hardware_destructive_interference_size.hpp"
#include "neutron/detail/utility/macros.hpp"

namespace neutron {

template <auto Next>
class intrusive_spsc_queue;

/**
 * @brief An intrusive, wait-free Single-Producer Single-Consumer (SPSC)
 * lock-free queue.
 *
 * Exactly one thread may call `push_back`, and exactly one (other) thread may
 * call `front`, `empty` and `pop_front`. Like other SPSC queues, the two ends
 * only communicate through the `Next` link, which is published with a release
 * store and observed with an acquire load.
 *
 * The queue keeps a private stub node as the sentinel in front of the first
 * element. When the last element is popped, the stub is linked again, which
 * keeps `pop_front` free of compare-and-swap loops.
 *
 * @tparam Node The type of the node stored in the queue. It's a member of the
 * queue as the stub, so it must be default constructible.
 * @tparam Next A pointer-to-member representing the `std::atomic<Node*>` link
 * inside the Node.
 */
template <typename Node, std::atomic<Node*> Node::* Next>
class intrusive_spsc_queue<Next> {
public:
    intrusive_spsc_queue() noexcept : head_(&stub_), tail_(&stub_) {
        (stub_.*Next).store(nullptr, std::memory_order_relaxed);
    }

    intrusive_spsc_queue(const intrusive_spsc_queue&)            = delete;
    intrusive_spsc_queue& operator=(const intrusive_spsc_queue&) = delete;
    intrusive_spsc_queue(intrusive_spsc_queue&&)                 = delete;
    intrusive_spsc_queue& operator=(intrusive_spsc_queue&&)      = delete;
    ~intrusive_spsc_queue() noexcept                             = default;

    /**
     * @brief Pushes a node to the back of the queue.
     *
     * Only the single producer thread may call this function. The node's `Next`
     * link is overwritten, so the node must not be in another intrusive
     * container.
     *
     * @param node Pointer to the node to be pushed.
     * @return true if the queue was empty before the push.
     * @return false if there were already elements in the queue.
     */
    auto push_back(Node* node) noexcept -> bool {
        (node->*Next).store(nullptr, std::memory_order_relaxed);
        Node* const prev = tail_.exchange(node, std::memory_order_acq_rel);
        (prev->*Next).store(node, std::memory_order_release);
        return prev == &stub_;
    }

    /**
     * @brief Returns the front node without removing it.
     *
     * Only the single consumer thread may call this function. The returned node
     * is still owned by the queue and remains valid until `pop_front` returns
     * it.
     *
     * @return The front node, or nullptr if the queue is empty. The result can
     * also be nullptr while a producer is in the middle of `push_back`.
     */
    ATOM_NODISCARD auto front() const noexcept -> Node* {
        Node* const head = head_;
        if (head == &stub_) {
            return (head->*Next).load(std::memory_order_acquire);
        }
        return head;
    }

    /**
     * @brief Checks whether the queue is empty.
     *
     * Only the single consumer thread may call this function.
     */
    ATOM_NODISCARD auto empty() const noexcept -> bool {
        return front() == nullptr;
    }

    /**
     * @brief Pops the front node from the queue.
     *
     * Only the single consumer thread may call this function. The node is
     * unlinked from the queue, so the caller is free to reuse or destroy it.
     *
     * @return The front node, or nullptr if the queue is empty. A producer in
     * the middle of `push_back` can also make this function return nullptr even
     * though a node is on its way.
     */
    auto pop_front() noexcept -> Node* {
        Node* head = head_;
        Node* next = (head->*Next).load(std::memory_order_acquire);

        if (head_ == &stub_) {
            if (next == nullptr) {
                return nullptr;
            }

            head_ = next;
            head  = next;
            next  = (head->*Next).load(std::memory_order_acquire);
        }

        if (next != nullptr) {
            head_ = next;
            return head;
        }

        // 1) `head` is the last element of the queue
        // 2) a producer is in the middle of `push_back`
        if (tail_.load(std::memory_order_relaxed) != head) {
            return nullptr;
        }

        // `head` is the last element: link the stub behind it, so that a
        // concurrent producer can publish the next node without touching us.
        push_back(&stub_);

        next = (head->*Next).load(std::memory_order_acquire);
        if (next != nullptr) {
            head_ = next;
            return head;
        }

        return nullptr;
    }

private:
    /// @brief The sentinel in front of the first element.
    Node stub_;

    /// @brief The first node that has not been popped yet, or `&stub_`.
    alignas(hdi_size) Node* head_;

    /// @brief The last node that has been pushed, or `&stub_`.
    alignas(hdi_size) std::atomic<Node*> tail_;
};

} // namespace neutron
