#ifdef ATOM_USES_MODULES
import std;
import neutron;
#else
    #include <atomic>
    #include <cstddef>
    #include <cstdlib>
    #include <memory>
    #include <new>
    #include <neutron/ecs.hpp>
#endif
#include "require.hpp"
#if defined(_WIN32)
    #ifndef ATOM_USES_MODULES
        #include <malloc.h>
    #endif
#endif

using namespace neutron;

namespace {

// command_buffer allocates its blocks with this alignment; nothing else in
// these tests does, so counting it counts block allocations.
constexpr std::align_val_t block_align{ 16 };
std::atomic<int> block_allocations = 0;

void* aligned_allocate(std::size_t size, std::size_t alignment) noexcept {
#if defined(_WIN32)
    // The Windows CRT has no std::aligned_alloc.
    return ::_aligned_malloc(size, alignment);
#else
    const auto rounded = (size + alignment - 1) / alignment * alignment;
    return std::aligned_alloc(alignment, rounded);
#endif
}

void aligned_deallocate(void* ptr) noexcept {
#if defined(_WIN32)
    ::_aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

} // namespace

void* operator new(std::size_t size, std::align_val_t align) {
    if (align == block_align) {
        block_allocations.fetch_add(1, std::memory_order_relaxed);
    }
    if (void* ptr = aligned_allocate(size, static_cast<std::size_t>(align))) {
        return ptr;
    }
    throw std::bad_alloc{};
}

void operator delete(void* ptr, std::align_val_t) noexcept {
    aligned_deallocate(ptr);
}

void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept {
    aligned_deallocate(ptr);
}

namespace {

struct Position {
    using component_concept = component_tag;
    float x{ 0 };
};

using world_t = basic_world<world_descriptor_t<>>;

std::size_t count_positions(world_t& world) {
    basic_querior<std::allocator<std::byte>, with<Position&>> querior{ world };
    query<with<Position&>> qry{ querior };
    std::size_t count = 0;
    for (auto [position] : qry.get()) {
        static_cast<void>(position);
        ++count;
    }
    return count;
}

void test_apply_runs_recorded_commands() {
    world_t world;
    command_buffer<> buffer;
    const auto entity = buffer.spawn(Position{ 1 });
    buffer.kill(entity);
    buffer.spawn(Position{ 2 });

    buffer.apply(world);

    require(count_positions(world) == 1);
}

void test_apply_leaves_the_buffer_empty() {
    world_t world;
    command_buffer<> buffer;
    buffer.spawn(Position{});

    buffer.apply(world);
    buffer.apply(world);

    require(count_positions(world) == 1);
}

void test_blocks_are_reused_across_frames() {
    world_t world;
    command_buffer<> buffer;
    // Enough commands to span several blocks.
    constexpr int commands_per_frame = 4000;
    int allocations[3]{};

    for (int& allocated : allocations) {
        const int before = block_allocations.load();
        for (int i = 0; i < commands_per_frame; ++i) {
            buffer.spawn(Position{ static_cast<float>(i) });
        }
        buffer.apply(world);
        allocated = block_allocations.load() - before;
    }

    require(allocations[0] > 1);
    // The same workload fits in the blocks kept from the first frame.
    require(allocations[1] == 0);
    require(allocations[2] == 0);
    require(count_positions(world) == 3 * commands_per_frame);
}

void test_command_list_records_into_a_command_buffer() {
    world_t world;
    command_buffer<> buffer;
    command_list list{ buffer };

    list.spawn(Position{ 3 });
    list.spawn(Position{ 4 });

    buffer.apply(world);
    require(count_positions(world) == 2);
}

} // namespace

int main() {
    test_apply_runs_recorded_commands();
    test_apply_leaves_the_buffer_empty();
    test_blocks_are_reused_across_frames();
    test_command_list_records_into_a_command_buffer();
    return neutron_test::result();
}
