#pragma once
#include <memory>
#include "../src/neutron/internal/allocator.hpp"
#include "../src/neutron/internal/macros.hpp"

namespace neutron {

template <typename Ty, _std_simple_allocator Alloc = std::allocator<Ty>>
class lifo_queue {
    template <typename T>
    using _allocator_t = rebind_alloc_t<Alloc, T>;

public:
    using value_type      = Ty;
    using allocator_type  = _allocator_t<Ty>;
    using alloc_traits    = std::allocator_traits<allocator_type>;
    using pointer         = typename alloc_traits::pointer;
    using const_pointer   = typename alloc_traits::const_pointer;
    using reference       = Ty&;
    using const_reference = const Ty&;
    using size_type       = typename alloc_traits::size_type;
    using difference_type = typename alloc_traits::difference_type;

    template <typename Al = Alloc>
    explicit constexpr lifo_queue(
        size_type num_blocks, size_type block_size, const Al& alloc = {});

    constexpr auto pop_back();

    constexpr auto push_back(const Ty&);

    constexpr auto push_back(Ty&&);

    template <typename ForwardIterator, typename Sentinel>
    constexpr auto push_back(ForwardIterator first, Sentinel last);

    NODISCARD auto block_size() const noexcept -> size_type;

    NODISCARD auto num_blocks() const noexcept -> size_type;
};

} // namespace neutron
