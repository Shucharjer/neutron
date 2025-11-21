#pragma once
#include <concepts>
#include <initializer_list>
#include <limits>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "neutron/memory.hpp"
#include "neutron/packet_uint.hpp"
#include "pair.hpp"
#include "../src/neutron/internal/const_identity.hpp"
#include "../src/neutron/internal/iterator/concepts.hpp"
#include "../src/neutron/internal/macros.hpp"
#include "../src/neutron/internal/mask.hpp"

#if HAS_CXX23
    #include "../src/neutron/internal/map_like.hpp"
#endif

namespace neutron {

/**
 * @brief A sparse-dense map optimized for ECS entity-component lookup with
 * generation-aware identities. This container provides O(1) average loopup and
 * fast iteration by splitting keys into high-order "generation" and low-order
 * "real key".
 *
 * Memory layout:
 *   - dense_:  contiguous vector of (key, value) pairs
 *   - sparse_: vector of unique_storage arrays (pages), each holding compated
 * key halves
 *
 * @tparam Kty      Unsigned integral type for keys (e.g. uint64_t)
 * @tparam Ty       Value type stored
 * @tparam Alloc    Allocator type for internal container (default:
 * std::allocator)
 * @tparam PageSize Number of entries per sparse page (must be power of two)
 */
template <
    std::unsigned_integral Kty, typename Ty, size_t PageSize = 32UL,
    size_t Shift                = sizeof(Kty) * 4UL,
    _std_simple_allocator Alloc = std::allocator<std::pair<Kty, Ty>>>
class shift_map {
public:
    static_assert(PageSize, "page size should not be zero");
    static_assert(
        Shift != 0 && Shift != (sizeof(Kty) * 8UL),
        "in this case, you may have other solution");

    constexpr static size_t total_bits = sizeof(Kty) * 8UL;

    template <typename T>
    using _allocator_t = rebind_alloc_t<Alloc, T>;

    template <typename T>
    using _vector_t = std::vector<T, _allocator_t<T>>;

    // clang-format off

    using key_type               = Kty;
    using mapped_type            = Ty;

    using value_type             = std::pair<const key_type, mapped_type>;
    using _value_type            = std::pair<const_identity<key_type>, mapped_type>;

    using _dense_type            = _vector_t<_value_type>;
    using _dense_alloc_t         = _allocator_t<_value_type>;
    using _dense_alloc_traits_t  = std::allocator_traits<_dense_alloc_t>;

    using allocator_type         = _dense_alloc_t;
    using alloc_traits           = _dense_alloc_traits_t;

    using size_type              = Kty; // may smaller than typename alloc_traits::size_type
    using difference_type        = typename alloc_traits::difference_type;
    using pointer                = typename alloc_traits::pointer;
    using const_pointer          = typename alloc_traits::const_pointer;
    using reference              = value_type&;
    using const_reference        = const value_type&;

    // the storage is effective when relocating, so we do not store the array directly.

    using _kept_type             = packed_uint_t<(1ULL << (total_bits - Shift)) - 1ULL>;
    using _array_t               = std::array<_kept_type, PageSize>;
    using _storage_t             = unique_storage<_array_t, _allocator_t<_array_t>>;
    using _sparse_type           = _vector_t<_storage_t>;
    using _sparse_alloc_t        = _allocator_t<_storage_t>;
    using _sparse_alloc_traits   = std::allocator_traits<_sparse_alloc_t>;

    using iterator               = typename _dense_type::iterator;
    using const_iterator         = typename _dense_type::const_iterator;
    using reverse_iterator       = typename _dense_type::reverse_iterator;
    using const_reverse_iterator = typename _dense_type::const_reverse_iterator;

    // clang-format on

    constexpr shift_map() = default;

    constexpr shift_map(const allocator_type& alloc)
        : dense_(alloc), sparse_(alloc) {}

    constexpr shift_map(
        [[maybe_unused]] std::allocator_arg_t, const allocator_type& alloc)
        : dense_(alloc), sparse_(alloc) {}

    template <input_iterator Iter, typename Sentinel>
    constexpr shift_map(
        Iter first, Sentinel last,
        const allocator_type& alloc = allocator_type{})
        : dense_(first, last, alloc), sparse_(alloc) {
        _set_sparse(0);
    }

    template <input_iterator Iter, typename Sentinel>
    constexpr shift_map(
        [[maybe_unused]] std::allocator_arg_t, const allocator_type& alloc,
        Iter first, Sentinel last)
        : shift_map(first, last, alloc) {}

    template <_pair Pair = value_type>
    constexpr shift_map(
        std::initializer_list<Pair> il, const allocator_type& alloc)
        : dense_(il.begin(), il.end(), alloc), sparse_(alloc) {
        _set_sparse(0);
    }

    template <_pair Pair = value_type>
    constexpr shift_map(std::initializer_list<Pair> il)
        : dense_(il.begin(), il.end(), allocator_type{}),
          sparse_(_sparse_alloc_t{}) {
        _set_sparse(0);
    }

    template <_pair Pair = value_type>
    constexpr shift_map(
        [[maybe_unused]] std::allocator_arg_t, const allocator_type& alloc,
        std::initializer_list<Pair> list)
        : shift_map(list, alloc) {}

#if HAS_CXX23
    template <std::ranges::range Rng>
    constexpr shift_map(
        [[maybe_unused]] std::from_range_t, Rng&& range,
        const allocator_type& alloc = allocator_type{})
        : dense_(std::from_range, std::forward<Rng>(range), alloc),
          sparse_(alloc) {
        if constexpr (map_like<Rng>) {
            _set_sparse_unique(0);
        } else {
            _set_sparse(0);
        }
    }
#endif

    constexpr shift_map(const shift_map& that)
        : dense_(that.dense_, that.get_allocator()),
          sparse_(that.get_allocator()) {
        _set_sparse(0);
    }

    constexpr shift_map(shift_map&& that) noexcept
        : dense_(std::move(that.dense_)), sparse_(std::move(that.sparse_)) {}

    constexpr shift_map(shift_map&& that, const allocator_type& alloc)
        : dense_(std::move(that.dense_), alloc),
          sparse_(std::move(that.sparse_), alloc) {}

    constexpr shift_map(
        [[maybe_unused]] std::allocator_arg_t, const allocator_type& alloc,
        shift_map&& that)
        : shift_map(std::move(that), alloc) {}

    constexpr shift_map& operator=(const shift_map& that) {
        if (this != &that) [[unlikely]] {
            dense_ = that.dense_;
            for (auto& storage : sparse_) {
                storage = _storage_t{ immediately };
            }
            _set_sparse(0);
        }
        return *this;
    }

    constexpr shift_map& operator=(shift_map&& that) noexcept {
        if (this != &that) [[unlikely]] {
            dense_  = std::move(that.dense_);
            sparse_ = std::move(that.sparse_);
        }
        return *this;
    }

    constexpr ~shift_map() noexcept = default;

    constexpr std::pair<iterator, bool> insert(const value_type& val) {
        return try_emplace(val.first, val.second);
    }

    constexpr std::pair<iterator, bool> insert(value_type&& val) {
        return try_emplace(val.first, std::move(val.second));
    }

#if HAS_CXX23
    template <typename Rng>
    constexpr void insert_range(Rng&& range) {
        const auto size = dense_.size();
        auto dense_guard =
            make_exception_guard([this, size] { dense_.resize(size); });
        dense_.insert_range(std::forward<Rng>(range));
        _set_sparse(size); // elements may not differs from elements in range
        dense_guard.mark_complete();
    }
#endif

    template <typename... Args>
    constexpr std::pair<iterator, bool> emplace(Args&&... args) {
        value_type pair(std::forward<Args>(args)...);
        return try_emplace(pair.first, std::move(pair.second));
    }

    template <typename... Args>
    constexpr std::pair<iterator, bool>
        try_emplace(key_type key, Args&&... args) {
        const _kept_type kept = _kept(key);
        const auto page       = _page_of(kept);
        const auto offset     = _offset_of(kept);

        if (_contains(key, page, offset)) {
            return std::make_pair(dense_.end(), false);
        }

        return _emplace_one_at_back(
            key, kept, page, offset, std::forward<Args>(args)...);
    }

    constexpr iterator erase(const_iterator where) noexcept {
        if (where == dense_.end()) [[unlikely]] {
            return where;
        }
        return erase(where->first);
    }

    constexpr iterator erase(key_type key) noexcept {
        const auto kept   = _kept(key);
        const auto page   = _page_of(kept);
        const auto offset = _offset_of(kept);
        if (_contains(key, page, offset)) [[likely]] {
            return _erase_at(kept, page, offset);
        }
        return dense_.end();
    }

    constexpr void reserve(size_type size) {
        dense_.reserve(size);
        _check_page(_page_of(_kept(size)));
    }

    NODISCARD constexpr iterator find(key_type key) noexcept {
        const auto kept   = _kept(key);
        const auto page   = _page_of(kept);
        const auto offset = _offset_of(kept);
        if (_contains(key, page, offset)) {
            return dense_.begin() + sparse_[page]->at(offset);
        }
        return dense_.end();
    }

    NODISCARD constexpr const_iterator find(key_type key) const noexcept {
        const auto kept   = _kept(key);
        const auto page   = _page_of(kept);
        const auto offset = _offset_of(kept);
        if (_contains(key, page, offset)) {
            return dense_.begin() + sparse_[page]->at(offset);
        }
        return dense_.end();
    }

    NODISCARD constexpr mapped_type& at(key_type key) {
        const auto kept   = _kept(key);
        const auto page   = _page_of(kept);
        const auto offset = _offset_of(kept);
        if (!_contains(key, page, offset)) [[unlikely]] {
            throw std::out_of_range("shift_map::at: key not found");
        }
        return dense_[sparse_[page]->at(offset)].second;
    }

    NODISCARD constexpr const mapped_type& at(key_type key) const {
        const auto kept   = _kept(key);
        const auto page   = _page_of(kept);
        const auto offset = _offset_of(kept);
        if (!_contains(key, page, offset)) [[unlikely]] {
            throw std::out_of_range("shift_map::at: key not found");
        }
        return dense_[sparse_[page]->at(offset)].second;
    }

    constexpr mapped_type& operator[](key_type key) {
        const auto kept   = _kept(key);
        const auto page   = _page_of(kept);
        const auto offset = _offset_of(kept);
        if (!_contains(key, page, offset)) {
            // requires default constructible
            _emplace_one_at_back(key, kept, page, offset);
        }
        return dense_[sparse_[page]->at(offset)].second;
    }

    NODISCARD constexpr bool contains(key_type key) const noexcept {
        const auto kept   = _kept(key);
        const auto page   = _page_of(kept);
        const auto offset = _offset_of(kept);
        return _contains(key, page, offset);
    }

    NODISCARD constexpr iterator begin() noexcept { return dense_.begin(); }
    NODISCARD constexpr const_iterator begin() const noexcept {
        return dense_.begin();
    }
    NODISCARD constexpr const_iterator cbegin() const noexcept {
        return dense_.cbegin();
    }

    NODISCARD constexpr iterator end() noexcept { return dense_.end(); }
    NODISCARD constexpr const_iterator end() const noexcept {
        return dense_.end();
    }
    NODISCARD constexpr const_iterator cend() const noexcept {
        return dense_.cend();
    }

    NODISCARD constexpr reverse_iterator rbegin() noexcept {
        return dense_.rbegin();
    }
    NODISCARD constexpr const_reverse_iterator rbegin() const noexcept {
        return dense_.rbegin();
    }
    NODISCARD constexpr const_reverse_iterator crbegin() const noexcept {
        return dense_.crbegin();
    }

    NODISCARD constexpr reverse_iterator rend() noexcept {
        return dense_.rend();
    }
    NODISCARD constexpr const_reverse_iterator rend() const noexcept {
        return dense_.rend();
    }
    NODISCARD constexpr const_reverse_iterator crend() const noexcept {
        return dense_.crend();
    }

    NODISCARD constexpr bool empty() const noexcept { return dense_.empty(); }

    NODISCARD constexpr size_t size() const noexcept { return dense_.size(); }

    NODISCARD constexpr size_t capacity() const noexcept {
        return dense_.capacity();
    }

    NODISCARD constexpr allocator_type& get_allocator() noexcept {
        return dense_.get_allocator();
    }

private:
    constexpr static _kept_type _kept(key_type key) noexcept {
        return static_cast<_kept_type>(
            key & (std::numeric_limits<_kept_type>::max)());
    }

    constexpr static size_type _page_of(_kept_type kept) noexcept {
        return _uint_dev<PageSize>(kept);
    }

    constexpr static size_type _offset_of(_kept_type kept) noexcept {
        return _uint_mod<PageSize>(kept);
    }

    constexpr bool _contains(
        // NOLINTNEXTLINE: internal implementation
        key_type key, size_type page, size_type offset) const noexcept {
        if (dense_.empty() || sparse_.size() <= page) {
            return false;
        }

        const auto index = sparse_[page]->at(offset);
        return dense_[index].first == key;
    }

    template <typename... Args>
    constexpr std::pair<iterator, bool> _emplace_one_at_back(
        key_type key, _kept_type kept, size_type page, size_type offset,
        Args&&... args) {
        auto dense_guard = make_exception_guard([this] { dense_.pop_back(); });
        const auto index = dense_.size();
        dense_.emplace_back(
            std::piecewise_construct, std::forward_as_tuple(key),
            std::forward_as_tuple(std::forward<Args>(args)...));
        _set_index(kept, page, offset, index);
        dense_guard.mark_complete();
        return std::make_pair(dense_.begin() + index, true);
    }

    constexpr void _set_index(
        _kept_type kept, size_type page, size_type offset, size_type index) {
        const auto current_page_count = sparse_.size();
        auto sparse_guard             = make_exception_guard(
            [this, current_page_count] { _pop_page_to(current_page_count); });
        _check_page(page);
        sparse_[page]->at(offset) = index;
        sparse_guard.mark_complete();
    }

    constexpr iterator
        _erase_at(_kept_type kept, size_type page, size_type offset) noexcept {
        auto& index       = sparse_[page]->at(offset);
        auto& back        = dense_.back();
        const auto backup = index;

        const auto back_kept = _kept(back.first);
        sparse_[_page_of(back_kept)]->at(_offset_of(back_kept)) = index;
        std::swap(dense_[index], back);
        dense_.pop_back();
        index = 0; // invaild index
        return dense_.begin() + backup;
    }

    constexpr void _pop_page_to(size_type page) noexcept {
        while (sparse_.size() != page) {
            sparse_.pop_back();
        }
    }

    constexpr void _check_page(size_type page) {
        while (page >= sparse_.size()) {
            sparse_.emplace_back(immediately);
        }
    }

    constexpr void _set_sparse(size_t begin) {
        for (size_t i = begin; i < dense_.size(); ++i) {
            const auto kept   = _kept(dense_[i].first);
            const auto page   = _page_of(kept);
            const auto offset = _offset_of(kept);
            if (!_contains(dense_[i].first, page, offset)) [[likely]] {
                _set_index(kept, page, offset, i);
            }
        }
    }

    constexpr void _set_sparse_unique(size_t begin) {
        for (size_t i = begin; i < dense_.size(); ++i) {
            const auto kept   = _kept(dense_[i].first);
            const auto page   = _page_of(kept);
            const auto offset = _offset_of(kept);
            _set_index(kept, page, offset, i);
        }
    }

    _dense_type dense_;
    _sparse_type sparse_;
};

namespace pmr {

template <
    std::unsigned_integral Kty, typename Ty, size_t PageSize = 32,
    size_t Shift = sizeof(Kty) * 4UL>
using shift_map =
    shift_map<Kty, Ty, PageSize, Shift, std::pmr::polymorphic_allocator<>>;

}

} // namespace neutron

/*! @cond TURN_OFF_DOXYGEN */
namespace std {

template <
    unsigned_integral Kty, typename Ty, typename Alloc, size_t PageSize,
    size_t Shift>
struct uses_allocator<
    neutron::shift_map<Kty, Ty, PageSize, Shift, Alloc>, Alloc> :
    std::true_type {};

} // namespace std
/*! @endcond */
