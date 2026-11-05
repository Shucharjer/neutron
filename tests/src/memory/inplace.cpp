#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <neutron/memory.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Tracks constructions and destructions to catch leaks and double destroys.
struct counted {
    inline static int constructed = 0;
    inline static int destroyed   = 0;

    explicit counted(int value) : value(value) { ++constructed; }
    counted(const counted&)            = delete;
    counted& operator=(const counted&) = delete;
    ~counted() { ++destroyed; }

    int value;
};

struct alignas(32) over_aligned {
    char value;
};

template <typename Storage>
concept has_capacity = requires(const Storage& storage) {
    { storage.capacity() } -> std::same_as<std::size_t>;
    { storage.max_size() } -> std::same_as<std::size_t>;
};

template <typename Storage, typename T>
concept data_of = requires(Storage& storage, const Storage& cstorage) {
    { storage.data() } -> std::same_as<T*>;
    { cstorage.data() } -> std::same_as<const T*>;
};

// Default `Constexpr` follows trivial constructibility.
static_assert(
    std::is_same_v<inplace_storage<int, 4>, inplace_storage<int, 4, true>>);
static_assert(std::is_same_v<
              inplace_storage<std::string, 4>,
              inplace_storage<std::string, 4, false>>);

// Layout: no overhead over a plain array.
static_assert(sizeof(inplace_storage<int, 4>) == sizeof(int[4])); // NOLINT
static_assert(
    sizeof(inplace_storage<int, 4, false>) == sizeof(int[4]));    // NOLINT
static_assert(
    sizeof(inplace_storage<std::string, 3>) == 3 * sizeof(std::string));
static_assert(alignof(inplace_storage<std::string, 3>) == alignof(std::string));
static_assert(
    sizeof(inplace_storage<over_aligned, 3>) == 3 * sizeof(over_aligned));
static_assert(
    alignof(inplace_storage<over_aligned, 3>) == alignof(over_aligned));
static_assert(sizeof(inplace_storage<counted, 5>) == 5 * sizeof(counted));

// Zero capacity is empty and usable as a base for EBO.
static_assert(std::is_empty_v<inplace_storage<int, 0>>);
static_assert(std::is_empty_v<inplace_storage<std::string, 0>>);
static_assert(std::is_empty_v<inplace_storage<int, 0, false>>);

// Storage never constructs the elements.
static_assert(
    std::is_trivially_default_constructible_v<inplace_storage<std::string, 4>>);
static_assert(
    std::is_trivially_destructible_v<inplace_storage<std::string, 4>>);
static_assert(std::is_trivially_copyable_v<inplace_storage<std::string, 4>>);
static_assert(
    std::is_trivially_default_constructible_v<inplace_storage<int, 4, false>>);
static_assert(std::is_trivially_copyable_v<inplace_storage<int, 4, true>>);

// data() is typed and const-correct; capacity() and max_size() are size_t.
static_assert(data_of<inplace_storage<int, 4>, int>);
static_assert(data_of<inplace_storage<int, 4, false>, int>);
static_assert(data_of<inplace_storage<int, 0>, int>);
static_assert(data_of<inplace_storage<std::string, 4>, std::string>);
static_assert(has_capacity<inplace_storage<int, 4>>);
static_assert(has_capacity<inplace_storage<int, 4, false>>);
static_assert(has_capacity<inplace_storage<int, 0>>);

// Trivial storages are usable in constant evaluation.
static_assert(inplace_storage<int, 4>{}.capacity() == 4);
static_assert(inplace_storage<int, 4>{}.max_size() == 4);
static_assert(inplace_storage<int, 4, false>{}.capacity() == 4);
static_assert(inplace_storage<int, 0>{}.capacity() == 0);
static_assert(inplace_storage<int, 0>{}.max_size() == 0);
static_assert(inplace_storage<int, 0>{}.data() == nullptr);

/// True if a `Constexpr` storage starts zeroed and is writable in constant
/// evaluation.
constexpr bool value_initialized_storage() {
    inplace_storage<int, 4, true> storage;
    for (std::size_t i = 0; i != storage.capacity(); ++i) {
        if (storage.data()[i] != 0) {
            return false;
        }
    }

    storage.data()[2] = 7;

    const auto& cref = storage;
    return cref.data()[2] == 7 && cref.data() == storage.data();
}
static_assert(value_initialized_storage());

/// True if a non-`Constexpr` storage is writable in constant evaluation.
constexpr bool default_initialized_storage() {
    constexpr std::size_t size = 3;
    inplace_storage<char, size, false> storage;

    for (std::size_t i = 0; i != size; ++i) {
        storage.data()[i] = static_cast<char>('a' + i);
    }

    return storage.data()[0] == 'a' && storage.data()[2] == 'c';
}
static_assert(default_initialized_storage());

void test_trivial_storage_is_zeroed_and_copyable() {
    constexpr std::size_t size = 8;
    inplace_storage<std::uint64_t, size> storage;

    for (std::size_t i = 0; i != size; ++i) {
        require(storage.data()[i] == 0U);
        storage.data()[i] = i * i;
    }
    const auto copy = storage;

    for (std::size_t i = 0; i != size; ++i) {
        require(copy.data()[i] == i * i);
    }
    require(copy.data() != storage.data());
}

void test_strings_are_constructed_in_place() {
    constexpr std::size_t size        = 4;
    constexpr std::size_t base_length = 32;
    inplace_storage<std::string, size> storage;
    std::string* const data = storage.data();
    require(static_cast<void*>(data) == static_cast<void*>(&storage));

    for (std::size_t i = 0; i != size; ++i) {
        std::construct_at(
            data + i, base_length + i, static_cast<char>('a' + i));
    }

    const auto& cref = storage;
    require(cref.data() == data);
    for (std::size_t i = 0; i != size; ++i) {
        require(cref.data()[i].size() == base_length + i);
        require(cref.data()[i].front() == static_cast<char>('a' + i));
    }

    std::destroy_n(data, size);
}

void test_element_lifetime_is_managed_by_the_user() {
    constexpr int size     = 6;
    constexpr int reused   = 3;
    constexpr int replaced = 42;
    counted::constructed   = 0;
    counted::destroyed     = 0;
    {
        inplace_storage<counted, size> storage;
        require(counted::constructed == 0);

        for (int i = 0; i != size; ++i) {
            std::construct_at(storage.data() + i, i);
        }
        for (int i = 0; i != size; ++i) {
            require(storage.data()[i].value == i);
        }

        // Reuse a slot after destroying its element.
        std::destroy_at(storage.data() + reused);
        std::construct_at(storage.data() + reused, replaced);
        require(storage.data()[reused].value == replaced);

        std::destroy_n(storage.data(), size);
    }

    require(counted::constructed == size + 1);
    require(counted::destroyed == size + 1);
}

void test_over_aligned_slots_are_aligned() {
    constexpr std::size_t size = 3;
    inplace_storage<over_aligned, size> storage;

    for (std::size_t i = 0; i != size; ++i) {
        auto* const slot   = storage.data() + i;
        const auto address = reinterpret_cast<std::uintptr_t>(slot);
        require(address % alignof(over_aligned) == 0);
        std::construct_at(slot, over_aligned{ static_cast<char>(i) });
    }

    require(storage.data()[2].value == 2);
}

} // namespace

int main() {
    test_trivial_storage_is_zeroed_and_copyable();
    test_strings_are_constructed_in_place();
    test_element_lifetime_is_managed_by_the_user();
    test_over_aligned_slots_are_aligned();
    return neutron_test::result();
}
