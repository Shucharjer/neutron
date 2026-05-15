#pragma once
#include <atomic>
#include <concepts>

namespace neutron {

template <auto Next>
class intrusive_mpsc_queue;

/**
 * @brief An intrusive, wait-free Multi-Producer Single Consumer (MPSC)
 * lock-free queue.
 *
 * This queue allows multiple thread to push nodes concurrently, while only a
 * single thread consumes (pops) nodes. It is "intrusive" because the nodes must
 * contain a specific atomic pointer member for linking.
 *
 * @tparam Node The type of the node stored in the queue. Must be default
 * initializable.
 * @tparam Next A pointer-to-member representing the `std::atomic<Node*>` link
 * inside the Node.
 */
template <typename Node, std::atomic<Node*> Node::* Next>
requires std::default_initializable<Node>
class intrusive_mpsc_queue<Next> {
public:
    intrusive_mpsc_queue() noexcept {
        (stub_.*Next).store(nullptr, std::memory_order_release);
    }

    /**
     * @brief Pushes a new node to the back of the queue.
     *
     * This operation is thread-safe and can be called by multiple producers
     * concurrently.
     *
     * @param node Pointer to the node to be pushed. The node's `Next` pointer
     * will be set to nullptr.
     * @return true if the queue was previously empty.
     * @return false if there were already elements in the queue.
     */
    constexpr auto push_back(Node* node) noexcept -> bool {
        (node->*Next).store(nullptr, std::memory_order_relaxed);
        Node* prev = tail_.exchange(node, std::memory_order_acq_rel);
        (prev->*Next).store(node, std::memory_order_release);
        return prev == &stub_;
    }

    /**
     * @brief Pops the front node from the queue.
     *
     * This operation must only be called by a single consumer thread.
     * Handles the synchronization with producers who might be in the middle of
     * a push.
     *
     * @return Pointer to the popped node, or nullptr if the queue is empty.
     */
    constexpr auto pop_front() noexcept -> Node* {
        Node* head = head_;

        Node* next = (head->*Next).load(std::memory_order_acquire);

        // &stub_ <- head_
        // ???
        if (head_ == &stub_) { // truely empty or need advance
            if (next == nullptr) {
                return nullptr;
            }

            head_ = next;
            head  = next;
            next  = (next->*Next).load(std::memory_order_acquire);
        }

        if (next != nullptr) {
            head_ = next;
            return head;
        }

        // 1) there are no more nodes in the queue
        // 2) in the middle of `push_back`

        // case 2
        Node* const tail = tail_.load(std::memory_order_relaxed);
        if (head != tail) {
            return nullptr;
        }

        // case 1
        push_back(&stub_); // set empty

        // retry
        next = (head->*Next).load(std::memory_order_acquire);
        if (next != nullptr) {
            head_ = next;
            return head;
        }

        return nullptr;
    }

private:
    Node stub_;

    Node* head_{ &stub_ };

    std::atomic<Node*> tail_{ &stub_ };
};

} // namespace neutron
