#include <memory>
#include <memory_resource>
#include <string>
#include <neutron/memory.hpp>
#include <neutron/object_pool.hpp>
#include "require.hpp"

using namespace neutron;

void test_unique_storage();
void test_unique_storage_pmr();
void test_pools();

int main() {
    test_unique_storage();
    test_unique_storage_pmr();
    test_pools();

    return 0;
}

// uses std::allocator
void test_unique_storage() {
    // default ctor
    {
        constexpr unique_storage<int> storage;
        static_assert(!storage);
    }

    // ctor
    {
        constexpr unique_storage<int> storage;
        static_assert(!storage);

        constexpr auto result = [] {
            unique_storage<int> storage{ 2 };
            return *storage;
        }();
        static_assert(result == 2);
    }

    // move ctor
    {
        constexpr auto storage = [] {
            unique_storage<int> storage;
            auto mov = std::move(storage);
            return mov;
        }();
        static_assert(!static_cast<bool>(storage));

        constexpr auto storage2 = [] {
            unique_storage<int> storage{ 2 };
            auto mov = std::move(storage);
            return *mov;
        }();
        static_assert(storage2 == 2);
    }

    // move assign
    {
        constexpr auto storage = [] {
            unique_storage<int> storage;
            unique_storage<int> mov;
            mov = std::move(storage);
            return mov;
        }();

        constexpr auto result = [] {
            unique_storage<int> storage{ 1 };
            unique_storage<int> mov;
            mov = std::move(storage);
            return *mov;
        }();
        static_assert(result == 1);
    }

    // swap
    {
        constexpr auto storage = [] {
            unique_storage<int> left{ 2 };
            unique_storage<int> right{};
            left.swap(right);
            return left;
        }();

        constexpr auto result = [] {
            unique_storage<int> left{ 2 };
            unique_storage<int> right{ 4 };
            left.swap(right);
            return *left;
        }();
        static_assert(result == 4);
    }
}

// uses std::pmr::polymorphic_allocator
void test_unique_storage_pmr() {
    using pmr_alloc_t = std::pmr::polymorphic_allocator<>;
    pmr_alloc_t alloc;

    // default ctor
    {
        unique_storage<int, pmr_alloc_t> storage;
        require(!storage);
    }

    // ctor (immediately)
    {
        unique_storage<int, pmr_alloc_t> storage{ immediately };
        require(storage);
    }

    // ctor (immediately, alloc)
    {
        unique_storage<int, pmr_alloc_t> storage{ immediately, alloc };
        require(storage);
    }

    // ctor (std::allocator_arg_t, alloc, immediately_t)
    {
        unique_storage<int, pmr_alloc_t> storage{ std::allocator_arg, alloc,
                                                  immediately };
        require(storage);
    }

    // ctor (arg)
    {
        unique_storage<int, pmr_alloc_t> storage{ std::allocator_arg, alloc,
                                                  2 };
        require(*storage == 2);
    }

    // move ctor
    {
        auto storage = [&] {
            unique_storage<int, pmr_alloc_t> storage{ alloc };
            auto mov = std::move(storage);
            return mov;
        }();
        require(!static_cast<bool>(storage));

        auto storage2 = [&] {
            unique_storage<int, pmr_alloc_t> storage{ std::allocator_arg, alloc,
                                                      2 };
            auto mov = std::move(storage);
            return *mov;
        }();
        require(storage2 == 2);
    }

    // move assign
    {
        auto storage = [] {
            unique_storage<int, pmr_alloc_t> storage;
            unique_storage<int, pmr_alloc_t> mov;
            mov = std::move(storage);
            return mov;
        }();

        auto result = [] {
            unique_storage<int, pmr_alloc_t> storage{ 1 };
            unique_storage<int, pmr_alloc_t> mov;
            mov = std::move(storage);
            return *mov;
        }();
        require(result == 1);
    }

    // swap
    {
        auto storage = [] {
            unique_storage<int, pmr_alloc_t> left{ 2 };
            unique_storage<int, pmr_alloc_t> right{};
            left.swap(right);
            return left;
        }();

        auto result = [] {
            unique_storage<int, pmr_alloc_t> left{ 2 };
            unique_storage<int, pmr_alloc_t> right{ 4 };
            left.swap(right);
            return *left;
        }();
        require(result == 4);
    }
}

void test_pool_proxy();
void test_constcapacity_pool();
void test_runtime_pool();
void test_pools() {
    test_pool_proxy();
    test_constcapacity_pool();
    test_runtime_pool();
}
template <typename Val>
void use_pool(auto& pool) {
    auto* const block1 = pool.template take<Val>();
    std::construct_at(static_cast<Val*>(block1), "hello world!");
    auto* const block2 = pool.template take<Val>();
    std::construct_at(static_cast<Val*>(block1), "this is a string");
    pool.put(block1);
    auto* block3 = pool.template take<Val>();
    require(block1 == block3);
    pool.put(block2);
    pool.put(block3);
}
void test_pool_proxy() {
    using value_type     = std::string;
    constexpr auto size  = sizeof(value_type);
    constexpr auto count = 4;

    using allocator = std::allocator<value_type>;

    auto* const blocks = allocator{}.allocate(count);
    _pool_proxy<size> pool(static_cast<void*>(blocks), count);
    use_pool<value_type>(pool);
    allocator{}.deallocate(blocks, count);
}
void test_constcapacity_pool() {
    using value_type     = std::string;
    constexpr auto size  = sizeof(value_type);
    constexpr auto count = 4;

    constcapacity_pool<size, count, alignof(value_type)> pool;
    use_pool<value_type>(pool);
}
void test_runtime_pool() {
    using value_type     = std::string;
    constexpr auto size  = sizeof(value_type);
    constexpr auto count = 4;

    runtime_pool<size, alignof(value_type)> pool{ count };
    use_pool<value_type>(pool);
}
