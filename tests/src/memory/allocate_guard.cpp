#include "neutron/detail/utility/macros.hpp"
#ifdef ATOM_USES_MODULES
import std;
import neutron;
#else
    #include <cstddef>
    #include <cstdint>
    #include <memory>
    #include <memory_resource>
    #include <new>
    #include <type_traits>
    #include <neutron/memory.hpp>
#endif
#include "require.hpp"

using namespace neutron;

namespace {

/// Counts live allocations and fails the N-th one on request.
class counting_resource : public std::pmr::memory_resource {
public:
    int live      = 0;
    int calls     = 0;
    int fail_call = -1;

private:
    void* do_allocate(std::size_t bytes, std::size_t align) override {
        if (calls++ == fail_call) {
            throw std::bad_alloc{};
        }
        ++live;
        return std::pmr::new_delete_resource()->allocate(bytes, align);
    }

    void do_deallocate(
        void* ptr, std::size_t bytes, std::size_t align) override {
        --live;
        std::pmr::new_delete_resource()->deallocate(ptr, bytes, align);
    }

    bool do_is_equal(const memory_resource& that) const noexcept override {
        return this == &that;
    }
};

using alloc_t = std::pmr::polymorphic_allocator<std::byte>;

struct alignas(64) wide {
    std::byte bytes[64];
};

template <typename T>
void deallocate(alloc_t alloc, T* ptr, std::size_t n) {
    std::pmr::polymorphic_allocator<T>{ alloc }.deallocate(ptr, n);
}

void test_guard_owns_every_allocation_until_destroyed() {
    counting_resource resource;
    {
        auto guard =
            make_allocate_guard<int, double, wide>(8, alloc_t{ &resource });
        require(resource.live == 3);
        require(guard.get<0>() != nullptr);
        require(guard.get<1>() != nullptr);
        require(reinterpret_cast<std::uintptr_t>(guard.get<2>()) % 64 == 0);
        // Observing does not release.
        require(resource.live == 3);
    }
    require(resource.live == 0);
}

void test_failed_allocation_returns_earlier_ones() {
    for (int failing : { 0, 1, 2 }) {
        counting_resource resource;
        resource.fail_call = failing;
        bool thrown        = false;
        ATOM_TRY {
            auto guard =
                make_allocate_guard<int, double, wide>(8, alloc_t{ &resource });
            static_cast<void>(guard);
        }
        ATOM_CATCH(const std::bad_alloc&) { thrown = true; }
        require(thrown);
        require(resource.live == 0);
    }
}

void test_released_allocations_outlive_the_guard() {
    counting_resource resource;
    alloc_t alloc{ &resource };
    int* ints       = nullptr;
    double* doubles = nullptr;
    {
        auto guard = make_allocate_guard<int, double>(4, alloc);
        ints       = std::move(guard).get<0>();
    }
    // Only the released one survives.
    require(resource.live == 1);
    deallocate(alloc, ints, 4);

    {
        auto guard = make_allocate_guard<int, double>(4, alloc);
        apply_allocation(guard, ints, doubles);
    }
    require(resource.live == 2);
    require(ints != nullptr);
    require(doubles != nullptr);
    deallocate(alloc, ints, 4);
    deallocate(alloc, doubles, 4);
    require(resource.live == 0);
}

void test_structured_binding_takes_ownership() {
    counting_resource resource;
    alloc_t alloc{ &resource };
    {
        auto [ints, doubles] = make_allocate_guard<int, double>(2, alloc);
        static_assert(std::is_same_v<decltype(ints), int*>);
        require(resource.live == 2);
        deallocate(alloc, ints, 2);
        deallocate(alloc, doubles, 2);
    }
    require(resource.live == 0);
}

// The allocator and the count are stored once, not per allocated type, and
// an empty allocator takes no space.
static_assert(
    sizeof(allocate_guard<std::allocator<std::byte>, int, double, wide>) ==
    sizeof(std::size_t) + (3 * sizeof(void*)));

constexpr bool allocates_in_constant_evaluation() {
    auto guard  = make_allocate_guard<int, long>(4);
    int* ints   = std::construct_at(guard.get<0>(), 1);
    long* longs = std::construct_at(guard.get<1>() + 3, 2L);
    return *ints + *longs == 3;
}

static_assert(allocates_in_constant_evaluation());

} // namespace

int main() {
    test_guard_owns_every_allocation_until_destroyed();
    test_failed_allocation_returns_earlier_ones();
    test_released_allocations_outlive_the_guard();
    test_structured_binding_takes_ownership();
    return neutron_test::result();
}
