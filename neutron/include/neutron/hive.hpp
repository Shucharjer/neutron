#pragma once
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>
#include "../src/neutron/internal/allocator.hpp"
#include "../src/neutron/internal/macros.hpp"
#include "../src/neutron/internal/memory/freeable_bytes.hpp"

namespace neutron {

struct hive_limits {
    size_t min;
    size_t max;
};

template <typename Ty, bool IsConst>
class _hive_iterator;

template <typename Ty, typename Alloc = std::allocator<Ty>>
class hive {
    template <typename T>
    using _allocator_t = rebind_alloc_t<Alloc, T>;

public:
    using value_type             = Ty;
    using allocator_type         = _allocator_t<Ty>;
    using allocator_triats       = std::allocator_traits<allocator_type>;
    using pointer                = typename allocator_triats::pointer;
    using const_pointer          = typename allocator_triats::const_pointer;
    using reference              = Ty&;
    using const_reference        = const Ty&;
    using size_type              = typename allocator_triats::size_type;
    using difference_type        = typename allocator_triats::difference_type;
    using iterator               = _hive_iterator<Ty, false>;
    using const_iterator         = _hive_iterator<Ty, true>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    class _hive_chunk;
    using _chunk_type = _hive_chunk;

    constexpr static bool _trivially_relocatable =
        std::is_trivially_copyable_v<Ty>;

    template <typename Al = Alloc>
    constexpr hive(const Al& alloc = {});

    template <typename Al = Alloc>
    constexpr hive(hive_limits limits, const Al& alloc = {});

    template <typename Al = Alloc>
    constexpr hive(
        size_type fill_n, const Ty& value = {}, const Al& alloc = {});

    template <typename Al = Alloc>
    constexpr hive(
        size_type fill_n, const Ty& value, hive_limits limits,
        const Al& alloc = {});

    template <typename Al = Alloc>
    constexpr hive(
        std::initializer_list<Ty> il, hive_limits limits, const Al& alloc = {});

    constexpr hive(const hive&);

    constexpr hive(hive&& that) noexcept;

    template <typename ThatAl, typename Al = Alloc>
    constexpr hive(hive<ThatAl>&& that, const Al& alloc = {});

    constexpr hive& operator=(const hive& that);

    constexpr hive& operator=(hive&& that) noexcept;

    constexpr ~hive() noexcept;

    constexpr iterator insert(const Ty& elem);

    constexpr iterator insert(Ty&& elem);

    constexpr iterator
        insert([[maybe_unused]] const_iterator hint, const Ty& elem);

    constexpr iterator insert([[maybe_unused]] const_iterator hint, Ty&& elem);

    template <typename... Args>
    constexpr iterator emplace(Args&&... args);

    template <typename... Args>
    constexpr iterator
        emplace_hint([[maybe_unused]] const_iterator hint, Args&&... args);

    constexpr const_iterator erase(const_iterator where);

private:
    template <typename... Args>
    constexpr iterator _emplace_one(Args&&...) noexcept(
        std::is_nothrow_constructible_v<Ty, Args...>);

    template <typename... Args>
    constexpr iterator _emplace_in_new(Args&&... args);

    size_t size_;
    size_t capacity_{};
    hive_limits limits_;

    using chunk_list = std::vector<_chunk_type, _allocator_t<_chunk_type>>;
    chunk_list chunks_;
    chunk_list::iterator insertible_chunk_;
};

template <typename Ty, typename Alloc>
class hive<Ty, Alloc>::_hive_chunk {
public:
    using bytes_type     = freeable_bytes<sizeof(Ty), alignof(Ty)>;
    using _bytes_alloc_t = rebind_alloc_t<Alloc, bytes_type>;

    template <typename Al>
    constexpr _hive_chunk(size_type n, const Al& alloc)
        : pool_(_bytes_alloc_t{ alloc }.allocate(n)), data_() {
        //
    }

    constexpr _hive_chunk(const _hive_chunk&);
    constexpr _hive_chunk(_hive_chunk&&) noexcept;
    constexpr _hive_chunk& operator=(const _hive_chunk&);
    constexpr _hive_chunk& operator=(_hive_chunk&&) noexcept;

    template <typename Al, typename... Args>
    auto emplace(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<Ty, Args...>) {
        //
    }

    constexpr auto erase();

    /// @brief Destroy allocated resources.
    /// @note The action is as same as dtor, so marked as `noexcept`.
    /// @warning It do not hold allocator, make sure call this before dtor.
    template <typename Al>
    constexpr auto destroy(const Al& alloc) noexcept {
        assert(pool_ != nullptr);

        // TODO: destroy elements
        static_assert(false);

        _bytes_alloc_t{ alloc }.deallocate(pool_, capacity_);

        pool_ = nullptr;
    }

    constexpr ~_hive_chunk() noexcept { assert(pool_ == nullptr); }

private:
    NODISCARD Ty* _at(difference_type n) noexcept {
        return reinterpret_cast<Ty*>(&data_[n]->bytes);
    }

    size_t size_{};
    size_t capacity_{};
    bytes_type* pool_;
    bytes_type** data_;
};

template <typename Ty, typename Alloc>
template <typename Al>
constexpr hive<Ty, Alloc>::hive(const Al& alloc)
    : chunks_(alloc), insertible_chunk_(chunks_.begin()) {}

template <typename Ty, typename Alloc>
template <typename Al>
constexpr hive<Ty, Alloc>::hive(hive_limits limits, const Al& alloc)
    : limits_(limits), chunks_(alloc), insertible_chunk_(chunks_.begin()) {}

template <typename Ty, typename Alloc>
template <typename... Args>
constexpr auto hive<Ty, Alloc>::emplace(Args&&... args) -> iterator {
    if (size_ != capacity_) [[likely]] {
        return _emplace_one(std::forward<Args>(args)...);
    } else {
        return _emplace_in_new(std::forward<Args>(args)...);
    }
}

template <typename Ty, typename Alloc>
template <typename... Args>
constexpr auto hive<Ty, Alloc>::_emplace_one(Args&&... args) noexcept(
    std::is_nothrow_constructible_v<Ty, Args...>) -> _hive_iterator<Ty, false> {
    insertible_chunk_.emplace();
}

template <typename Ty, typename Alloc>
template <typename... Args>
constexpr auto hive<Ty, Alloc>::_emplace_in_new(Args&&... args)
    -> _hive_iterator<Ty, false> {
    auto& chunk = chunks_.emplace_back();
}

} // namespace neutron
