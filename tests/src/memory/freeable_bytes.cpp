#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <neutron/memory.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Owns `Count` heap blocks chained by `_init_freeable_bytes`.
template <std::size_t Size, std::size_t Align, std::size_t Count>
struct linked_blocks {
    using block = freeable_bytes<Size, Align>;

    linked_blocks() { _init_freeable_bytes(storage.get(), Count); }

    ATOM_NODISCARD block* data() const noexcept { return storage.get(); }

    std::unique_ptr<block[]> storage = std::make_unique<block[]>(Count);
};

/// Takes every block of a fresh list, in list order.
template <std::size_t Size, std::size_t Align, std::size_t Count>
std::array<void*, Count> take_all(freeable_bytes<Size, Align>*& head) noexcept {
    std::array<void*, Count> taken{};
    for (auto& pointer : taken) {
        pointer = _take_bytes(head);
    }
    return taken;
}

// Layout: at least a pointer and Size bytes, rounded up to Align.
static_assert(sizeof(freeable_bytes<1>) == sizeof(void*));
static_assert(alignof(freeable_bytes<1>) == 8);
static_assert(sizeof(freeable_bytes<12>) == 16);
static_assert(sizeof(freeable_bytes<16>) == 16);
static_assert(sizeof(freeable_bytes<1, 32>) == 32);
static_assert(alignof(freeable_bytes<1, 32>) == 32);
static_assert(sizeof(freeable_bytes<33, 32>) == 64);
static_assert(alignof(freeable_bytes<100, 64>) == 64);
static_assert(sizeof(freeable_bytes<100, 64>) == 128);

// Blocks are raw storage.
static_assert(std::is_trivially_default_constructible_v<freeable_bytes<8>>);
static_assert(std::is_trivially_copyable_v<freeable_bytes<8>>);

/// Takes every block in constant evaluation; true if they come in array order.
template <std::size_t Size, std::size_t Align, std::size_t Count>
constexpr bool take_walks_list_in_constant_evaluation() {
    using block = freeable_bytes<Size, Align>;
    std::array<block, Count> blocks{};
    _init_freeable_bytes(blocks.data(), Count);
    block* head = blocks.data();

    for (std::size_t i = 0; i < Count; ++i) {
        if (_take_bytes(head) != static_cast<void*>(&blocks[i])) {
            return false;
        }
    }

    return head == nullptr && _take_bytes(head) == nullptr && head == nullptr;
}

// The free list is usable in constant evaluation.
static_assert(take_walks_list_in_constant_evaluation<8, 8, 1>());
static_assert(take_walks_list_in_constant_evaluation<24, 8, 5>());
static_assert(take_walks_list_in_constant_evaluation<1, 64, 3>());

template <std::size_t Size, std::size_t Align, std::size_t Count>
void test_init_links_blocks_in_address_order() {
    const linked_blocks<Size, Align, Count> list;
    auto* const blocks = list.data();

    for (std::size_t i = 0; i + 1 < Count; ++i) {
        require(blocks[i].next == &blocks[i + 1]);
    }
    require(blocks[Count - 1].next == nullptr);
}

template <std::size_t Size, std::size_t Align, std::size_t Count>
void test_take_returns_aligned_writable_blocks_in_order() {
    const linked_blocks<Size, Align, Count> list;
    auto* const blocks = list.data();
    auto* head         = blocks;

    for (std::size_t i = 0; i < Count; ++i) {
        void* const taken = _take_bytes(head);

        require(taken == &blocks[i]);
        require(reinterpret_cast<std::uintptr_t>(taken) % Align == 0);
        // Taken memory is fully writable.
        std::uninitialized_fill_n(
            static_cast<std::byte*>(taken), Size, std::byte{ 0xab });
    }

    require(head == nullptr);
    require(_take_bytes(head) == nullptr);
}

template <std::size_t Size, std::size_t Align, std::size_t Count>
void test_put_ignores_null() {
    const linked_blocks<Size, Align, Count> list;
    auto* head = list.data();
    take_all<Size, Align, Count>(head);

    _put_bytes(head, nullptr);

    require(head == nullptr);
}

template <std::size_t Size, std::size_t Align, std::size_t Count>
void test_put_and_take_behave_as_lifo_stack() {
    static_assert(Count >= 3, "the test returns three distinct blocks");
    const linked_blocks<Size, Align, Count> list;
    auto* head       = list.data();
    const auto taken = take_all<Size, Align, Count>(head);

    _put_bytes(head, taken[1]);
    _put_bytes(head, taken[0]);
    _put_bytes(head, taken[Count - 1]);

    require(_take_bytes(head) == taken[Count - 1]);
    require(_take_bytes(head) == taken[0]);

    _put_bytes(head, taken[2]);

    require(_take_bytes(head) == taken[2]);
    require(_take_bytes(head) == taken[1]);
    require(_take_bytes(head) == nullptr);
}

/// Runs every free list test for one block layout.
template <std::size_t Size, std::size_t Align, std::size_t Count>
void test_free_list() {
    test_init_links_blocks_in_address_order<Size, Align, Count>();
    test_take_returns_aligned_writable_blocks_in_order<Size, Align, Count>();
    test_put_ignores_null<Size, Align, Count>();
    test_put_and_take_behave_as_lifo_stack<Size, Align, Count>();
}

void test_init_uninitialized_links_raw_memory() {
    using block                 = freeable_bytes<40, 16>;
    constexpr std::size_t count = 4;
    constexpr std::align_val_t align{ alignof(block) };
    void* const raw = ::operator new(sizeof(block) * count, align);

    _init_uninitialized_freeable_bytes<block>(raw, count);

    auto* head = static_cast<block*>(raw);
    for (std::size_t i = 0; i < count; ++i) {
        void* const expected =
            static_cast<std::byte*>(raw) + (i * sizeof(block));
        require(_take_bytes(head) == expected);
    }
    require(head == nullptr);

    ::operator delete(raw, align);
}

} // namespace

int main() {
    test_free_list<8, 8, 4>();
    test_free_list<24, 8, 7>();
    test_free_list<1, 32, 3>();
    test_free_list<100, 64, 5>();
    test_init_uninitialized_links_raw_memory();
    return neutron_test::result();
}
