#pragma once
#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>
#include <neutron/memory.hpp>
#include "neutron/detail/lockfree/intrusive_mpsc_queue.hpp"
#include "neutron/detail/lockfree/node.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/memory/rebind_alloc.hpp"

namespace neutron {

template <typename T, typename Alloc = std::allocator<T>>
class mpsc_queue :
    private intrusive_mpsc_queue<&internal::_lockfree_atomic_node<T>::next> {
    using _node              = internal::_lockfree_atomic_node<T>;
    using _base              = intrusive_mpsc_queue<&_node::next>;
    using _node_alloc        = rebind_alloc_t<Alloc, _node>;
    using _node_alloc_traits = std::allocator_traits<_node_alloc>;

    static constexpr bool pass_by_value =
        std::is_trivially_copy_constructible_v<T> &&
        (sizeof(T) <= (sizeof(void*) << 1));

public:
    using value_type     = T;
    using allocator_type = rebind_alloc_t<Alloc, T>;
    using _alloc_traits  = std::allocator_traits<allocator_type>;

    mpsc_queue() noexcept(std::is_nothrow_default_constructible_v<Alloc>) =
        default;

    template <typename Al = Alloc>
    requires std::convertible_to<Al, Alloc>
    mpsc_queue(const Al& alloc) noexcept(
        std::is_nothrow_constructible_v<Alloc, Al>)
        : alloc_(alloc) {}

    mpsc_queue(const mpsc_queue& that) = delete;

    mpsc_queue(mpsc_queue&& that) noexcept = delete;

    mpsc_queue& operator=(const mpsc_queue& that) = delete;

    mpsc_queue& operator=(mpsc_queue&& that) noexcept = delete;

    ~mpsc_queue() {
        std::optional<T> opt;
        while (opt = pop_front(), opt.has_value()) {}
    }

    bool push_back(T val) noexcept
    requires pass_by_value
    {
        auto* node = _node_alloc_traits::allocate(alloc_, 1);
        _node_alloc_traits::construct(alloc_, node, val);
        return _base::push_back(node);
    }

    bool push_back(const T& val) noexcept
    requires(!pass_by_value)
    {
        auto* node = _node_alloc_traits::allocate(alloc_, 1);
        _node_alloc_traits::construct(alloc_, node, val);
        return _base::push_back(node);
    }

    bool push_back(T&& val) noexcept
    requires(!pass_by_value)
    {
        auto* node = _node_alloc_traits::allocate(alloc_, 1);
        _node_alloc_traits::construct(alloc_, node, std::move(val));
        return _base::push_back(node);
    }

    std::optional<T> pop_front() noexcept {
        _node* node = _base::pop_front();
        if (node != nullptr) {
            T val = std::move(node->value);
            _node_alloc_traits::deallocate(alloc_, node, 1);
            return val;
        }
        return {};
    }

private:
    ATOM_NO_UNIQUE_ADDR _node_alloc alloc_;
    static constexpr std::size_t block_size = 256;
    struct _block {
        using _freeable_bytes = freeable_bytes<sizeof(_node), alignof(_node)>;
        // NOLINTNEXTLINE(modernize-avoid-c-arrays)
        _freeable_bytes storage[block_size];
        _freeable_bytes* free;
    };
    _block* blocks_;
    _block* free_;
    std::size_t block_count_;
};

} // namespace neutron
