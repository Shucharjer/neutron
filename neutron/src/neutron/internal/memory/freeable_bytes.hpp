#pragma once
#include <cassert>
#include <cstddef>
#include <iterator>

namespace neutron {

/**
 * @brief A union stores bytes or next free memory block.
 * The size of a union equals to the largest element.
 * @tparam Size
 */
template <size_t Size, size_t Align = 8>
union alignas(Align) freeable_bytes {
    freeable_bytes* next;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    std::byte bytes[Size];
};

template <size_t Size>
constexpr void _init_freeable_bytes(
    freeable_bytes<Size>* blocks, size_t capacity) noexcept {
    assert(capacity != 0);
    for (size_t i = 0; i < capacity - 1; i++) {
        auto* const block = std::launder(std::next(blocks, i));
        block->next       = std::next(blocks, i + 1);
    }
    std::next(blocks, capacity - 1)->next = nullptr;
}

template <size_t Size>
constexpr void
    _init_uninitialized_freeable_bytes(void* blocks, size_t capacity) noexcept {
    static_assert(
        std::is_trivially_default_constructible_v<freeable_bytes<Size>>);
    _init_freeable_bytes<Size>(
        static_cast<freeable_bytes<Size>*>(blocks), capacity);
}

template <size_t Size>
constexpr void* _take_bytes(
    freeable_bytes<Size>* blocks, freeable_bytes<Size>*& head) noexcept {
    if (head == nullptr) [[unlikely]] {
        return nullptr;
    }
    auto* const block = head;
    head              = head->next;
    return block;
}

template <size_t Size>
constexpr void _put_bytes(
    freeable_bytes<Size>* blocks, freeable_bytes<Size>*& head, size_t capacity,
    void* pointer) noexcept {
    if (pointer == nullptr) [[unlikely]] {
        return;
    }
    auto* const block = static_cast<freeable_bytes<Size>*>(pointer);
    block->next       = head;
    head              = block;
}

} // namespace neutron
