#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <neutron/memory.hpp>

using namespace neutron;

namespace {

/// @brief Tracks live instances to catch leaks and double destroys.
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

constexpr bool value_initialized_storage() {
    inplace_storage<int, 4, true> storage;
    for (std::size_t i = 0; i != storage.capacity(); ++i) {
        if (storage.data()[i] != 0) {
            return false;
        }
    }
    storage.data()[2] = 7;
    const auto& cref  = storage;
    return cref.data()[2] == 7 && cref.data() == storage.data();
}
static_assert(value_initialized_storage());

constexpr bool default_initialized_storage() {
    inplace_storage<char, 3, false> storage;
    for (std::size_t i = 0; i != 3; ++i) {
        storage.data()[i] = static_cast<char>('a' + i);
    }
    return storage.data()[0] == 'a' && storage.data()[2] == 'c';
}
static_assert(default_initialized_storage());

bool test_trivial() {
    inplace_storage<std::uint64_t, 8> storage;
    for (std::size_t i = 0; i != 8; ++i) {
        if (storage.data()[i] != 0U) {
            return false;
        }
        storage.data()[i] = i * i;
    }
    const auto copy = storage;
    for (std::size_t i = 0; i != 8; ++i) {
        if (copy.data()[i] != i * i) {
            return false;
        }
    }
    return copy.data() != storage.data();
}

bool test_non_trivial_strings() {
    constexpr std::size_t size = 4;
    inplace_storage<std::string, size> storage;
    std::string* data = storage.data();
    if (static_cast<void*>(data) != static_cast<void*>(&storage)) {
        return false;
    }
    for (std::size_t i = 0; i != size; ++i) {
        std::construct_at(data + i, 32 + i, static_cast<char>('a' + i));
    }
    const auto& cref = storage;
    bool result      = cref.data() == data;
    for (std::size_t i = 0; i != size; ++i) {
        result = result && cref.data()[i].size() == 32 + i &&
                 cref.data()[i].front() == static_cast<char>('a' + i);
    }
    std::destroy_n(data, size);
    return result;
}

bool test_counted_lifetime() {
    counted::constructed = 0;
    counted::destroyed   = 0;
    {
        inplace_storage<counted, 6> storage;
        if (counted::constructed != 0) {
            return false;
        }
        for (int i = 0; i != 6; ++i) {
            std::construct_at(storage.data() + i, i);
        }
        for (int i = 0; i != 6; ++i) {
            if (storage.data()[i].value != i) {
                return false;
            }
        }
        // Reuse a slot after destroying its element.
        std::destroy_at(storage.data() + 3);
        std::construct_at(storage.data() + 3, 42);
        if (storage.data()[3].value != 42) {
            return false;
        }
        std::destroy_n(storage.data(), 6);
    }
    return counted::constructed == 7 && counted::destroyed == 7;
}

bool test_over_aligned() {
    inplace_storage<over_aligned, 3> storage;
    for (std::size_t i = 0; i != 3; ++i) {
        auto* slot = storage.data() + i;
        if (reinterpret_cast<std::uintptr_t>(slot) % alignof(over_aligned) !=
            0) {
            return false;
        }
        std::construct_at(slot, over_aligned{ static_cast<char>(i) });
    }
    return storage.data()[2].value == 2;
}

} // namespace

int main() {
    if (!test_trivial()) {
        return 1;
    }
    if (!test_non_trivial_strings()) {
        return 2;
    }
    if (!test_counted_lifetime()) {
        return 3;
    }
    if (!test_over_aligned()) {
        return 4;
    }
    return 0;
}
