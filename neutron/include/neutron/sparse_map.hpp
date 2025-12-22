#pragma once
#include <array>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <memory_resource>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "neutron/memory.hpp"
#include "neutron/pair.hpp"
#include "../src/neutron/internal/allocator.hpp"
#include "../src/neutron/internal/macros.hpp"
#include "../src/neutron/internal/mask.hpp"
#include "../src/neutron/internal/utility/const_identity.hpp"

#if HAS_CXX23
    #include "../src/neutron/internal/map_like.hpp"
    #include "../src/neutron/internal/ranges/concepts.hpp"
#endif

namespace neutron {

template <
    std::unsigned_integral Kty, typename Ty, std::size_t PageSize = 32UL,
    _std_simple_allocator Alloc = std::allocator<std::pair<const Kty, Ty>>>
class sparse_map {
public:
    static_assert(PageSize, "page size should not be zero");

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

    using _array_t               = std::array<key_type, PageSize>;
    using _storage_t             = unique_storage<_array_t, _allocator_t<_array_t>>;
    using _sparse_type           = _vector_t<_storage_t>;
    using _sparse_alloc_t        = _allocator_t<_storage_t>;
    using _sparse_alloc_traits   = std::allocator_traits<_sparse_alloc_t>;

    using iterator               = typename _dense_type::iterator;
    using const_iterator         = typename _dense_type::const_iterator;
    using reverse_iterator       = typename _dense_type::reverse_iterator;
    using const_reverse_iterator = typename _dense_type::const_reverse_iterator;

    // clang-format on

    /**
     * @brief Default constructor.
     *
     */
    constexpr sparse_map() noexcept(
        std::is_nothrow_default_constructible_v<_dense_type> &&
        std::is_nothrow_default_constructible_v<_sparse_type>)
        : dense_(), sparse_() {}

    /**
     * @brief Construct with allocator.
     *
     */
    template <typename Al>
    constexpr sparse_map(const Al& allocator) noexcept(
        std::is_nothrow_constructible_v<_dense_type, _dense_alloc_t> &&
        std::is_nothrow_constructible_v<_sparse_type, _sparse_alloc_t>)
        : dense_(allocator), sparse_(allocator) {}

    constexpr sparse_map(std::allocator_arg_t, const Alloc& alloc) noexcept(
        std::is_nothrow_constructible_v<sparse_map, Alloc>)
        : sparse_map(alloc) {}

    /**
     * @brief Construct by iterators and allocator.
     *
     */
    template <std::input_iterator Iter, typename Sentinel>
    constexpr sparse_map(Iter first, Sentinel last, const Alloc& alloc) noexcept(
        noexcept(sparse_map(std::allocator_arg, alloc, first, last)))
        : dense_(first, last, alloc), sparse_(alloc) {
        for (const auto& [key, val] : dense_) {
            auto page   = _page_of(key);
            auto offset = _offset_of(key);
            _check_page(page);
            sparse_[page]->at(offset) = dense_.size();
        }
    }

    /**
     * @brief Construct by iterators and allocator.
     *
     */
    template <std::input_iterator Iter, typename Sentinel>
    constexpr explicit sparse_map(
        std::allocator_arg_t, const Alloc& alloc, Iter first, Sentinel last)
        : sparse_map(first, last, alloc) {}

    /**
     * @brief Construct by iterators.
     *
     */
    template <std::input_iterator IFirst, typename Sentinel>
    constexpr sparse_map(IFirst first, Sentinel last)
        : sparse_map(first, last, _dense_alloc_t{}) {}

    /**
     * @brief Construct by initializer list and allocator.
     *
     */
    template <typename Al = Alloc, _pair Pair = value_type>
    requires std::convertible_to<Pair, value_type>
    constexpr sparse_map(
        std::initializer_list<Pair> list, const Al& allocator = Alloc{})
        : dense_(list.begin(), list.end(), allocator), sparse_(allocator) {
        for (auto i = 0; i < list.size(); ++i) {
            const auto& [key, val] = dense_[i];
            auto page              = _page_of(key);
            auto offset            = _offset_of(key);
            _check_page(page);
            sparse_[page]->at(offset) = i;
        }
    }

    template <typename Al, _pair Pair = value_type>
    requires std::convertible_to<Pair, value_type>
    constexpr sparse_map(
        std::allocator_arg_t, const Al& alloc, std::initializer_list<Pair> list)
        : sparse_map(list, alloc) {}

    constexpr sparse_map(sparse_map&& that) noexcept
        : dense_(std::move(that.dense_)), sparse_(std::move(that.sparse_)) {}

    constexpr sparse_map& operator=(sparse_map&& that) noexcept {
        if (this != &that) {
            dense_  = std::move(that.dense_);
            sparse_ = std::move(that.sparse_);
        }
        return *this;
    }

    constexpr sparse_map(const sparse_map& that)
        : dense_(that.dense_),
          sparse_(that.sparse_.size(), that.sparse_.get_allocator()) {
        for (const auto& page : that.sparse_) {
            sparse_.emplace_back(*page);
        }
    }

    constexpr sparse_map& operator=(const sparse_map& that) {
        if (this != &that) {
            sparse_map temp(that);
            std::swap(dense_, temp.dense_);
            std::swap(sparse_, temp.sparse_);
        }
        return *this;
    }

    constexpr ~sparse_map() noexcept = default;

    NODISCARD constexpr auto at(const key_type key) -> Ty& {
        auto page   = _page_of(key);
        auto offset = _offset_of(key);
        return dense_[sparse_[page]->at(offset)].second;
    }

    NODISCARD constexpr auto at(const key_type key) const -> const Ty& {
        auto page   = _page_of(key);
        auto offset = _offset_of(key);
        return dense_[sparse_[page]->at(offset)].second;
    }

    constexpr auto operator[](const key_type key) -> Ty& {
        auto page   = _page_of(key);
        auto offset = _offset_of(key);
        return dense_[sparse_[page]->at(offset)].second;
    }

    constexpr std::pair<iterator, bool> insert(const _value_type& val) {
        return try_emplace(val.first, val.second);
    }

    constexpr std::pair<iterator, bool> insert(_value_type&& val) {
        return try_emplace(val.first, std::move(val.second));
    }

    constexpr std::pair<iterator, bool> insert(const value_type& val) {
        return try_emplace(val.first, val.second);
    }

    constexpr std::pair<iterator, bool> insert(value_type&& val) {
        return try_emplace(val.first, std::move(val.second));
    }

#if HAS_CXX23
    template <compatible_range<value_type> Rng>
    constexpr void insert_range(Rng&& range) {
        const auto size = dense_.size();
        dense_.append_range(std::forward<Rng>(range));
        for (auto i = size; i < dense_.size(); ++i) {
            auto page   = _page_of(dense_[i].first);
            auto offset = _offset_of(dense_[i].first);
            _check_page(page);
            sparse_[page]->at(offset) = dense_.size();
        }
    }
#endif

    template <typename... Args>
    constexpr std::pair<iterator, bool> emplace(Args&&... args) {
        value_type pair(std::forward<Args>(args)...);
        return try_emplace(pair.first, std::move(pair.second));
    }

    template <typename... Args>
    constexpr std::pair<iterator, bool>
        try_emplace(const key_type key, Args&&... args) {
        const auto page   = _page_of(key);
        const auto offset = _offset_of(key);

        if (_contains_impl(key, page, offset)) {
            return std::make_pair(dense_.end(), false);
        }

        return _emplace_one_at_back(
            key, page, offset, std::forward<Args>(args)...);
    }

    /**
     * @note The defererence of `where` is unsafe, which the user should make
     * sure the iterator is vaild and do not equals to `end()`.
     */
    constexpr iterator erase(const_iterator where) {
        const auto key    = where->first; // deref
        const auto page   = _page_of(key);
        const auto offset = _offset_of(key);

        _erase_without_check_impl(page, offset);
    }

    constexpr iterator erase(const key_type key) {
        auto page   = _page_of(key);
        auto offset = _offset_of(key);

        if (_contains_impl(key, page, offset)) {
            return _erase_without_check_impl(page, offset);
        }
        return dense_.end();
    }

    constexpr auto erase_without_check(const key_type key) {
        auto page   = _page_of(key);
        auto offset = _offset_of(key);
        _erase_without_check_impl(page, offset);
    }

    constexpr void reserve(const size_type size) {
        auto page = _page_of(size);
        _check_page(page);
        dense_.reserve(size);
    }

    constexpr auto contains(const key_type key) const -> bool {
        auto page   = _page_of(key);
        auto offset = _offset_of(key);
        return _contains_impl(key, page, offset);
    }

    NODISCARD constexpr auto find(const key_type key) noexcept -> iterator {
        auto page   = _page_of(key);
        auto offset = _offset_of(key);

        if (_contains_impl(key, page, offset)) {
            return dense_.begin() + sparse_[page]->at(offset);
        }
        return dense_.end();
    }

    NODISCARD constexpr auto find(const key_type key) const noexcept
        -> const_iterator {
        auto page   = _page_of(key);
        auto offset = _offset_of(key);

        if (_contains_impl(key, page, offset)) {
            return dense_.begin() + sparse_[page]->at(offset);
        }
        return dense_.end();
    }

    NODISCARD constexpr bool empty() const noexcept { return dense_.empty(); }

    NODISCARD constexpr size_type size() const noexcept {
        return dense_.size();
    }

    constexpr void clear() noexcept {
        sparse_.clear();
        dense_.clear();
    }

    NODISCARD constexpr auto front() -> value_type& { return dense_.front(); }
    NODISCARD constexpr auto front() const -> const value_type& {
        return dense_.front();
    }

    NODISCARD constexpr auto back() -> value_type& { return dense_.back(); }
    NODISCARD constexpr auto back() const -> const value_type& {
        return dense_.back();
    }

    NODISCARD constexpr auto begin() noexcept -> iterator {
        return dense_.begin();
    }
    NODISCARD constexpr auto begin() const noexcept -> const_iterator {
        return dense_.begin();
    }
    NODISCARD constexpr auto cbegin() const noexcept -> const_iterator {
        return dense_.cbegin();
    }

    NODISCARD constexpr auto end() noexcept -> iterator { return dense_.end(); }
    NODISCARD constexpr auto end() const noexcept -> const_iterator {
        return dense_.end();
    }
    NODISCARD constexpr auto cend() const noexcept -> const_iterator {
        return dense_.cend();
    }

    NODISCARD constexpr auto rbegin() noexcept -> reverse_iterator {
        return dense_.rbegin();
    }
    NODISCARD constexpr auto rbegin() const noexcept -> const_reverse_iterator {
        return dense_.rbegin();
    }
    NODISCARD constexpr auto crbegin() const noexcept
        -> const_reverse_iterator {
        return dense_.crbegin();
    }

    NODISCARD constexpr auto rend() noexcept -> reverse_iterator {
        return dense_.rend();
    }
    NODISCARD constexpr auto rend() const noexcept -> const_reverse_iterator {
        return dense_.rend();
    }
    NODISCARD constexpr auto crend() const noexcept -> const_reverse_iterator {
        return dense_.crend();
    }

    constexpr void swap(sparse_map&) noexcept;

    NODISCARD constexpr auto get_allocator() const noexcept {
        return dense_.get_allocator();
    }

private:
    constexpr static size_type _page_of(const key_type key) noexcept {
        return _uint_dev<PageSize>(key);
    }
    constexpr static size_type _offset_of(const key_type key) noexcept {
        return _uint_mod<PageSize>(key);
    }

    constexpr void _check_page(const size_type page) {
        // after calling this function, sparse_.size() should be larger than
        // page

        if (sparse_.size() > page) [[likely]] {
            if (!static_cast<bool>(sparse_[page])) {
                sparse_[page] = _storage_t{ immediately };
            }

            return;
        }

        const auto current_page_count = sparse_.size();
        auto sparse_guard             = make_exception_guard(
            [this, current_page_count] { _pop_page_to(current_page_count); });

        if (sparse_.size() < page) {
            sparse_.resize(page); // default construct to page
        }

        sparse_.emplace_back(immediately); // make sure page is accessible

        sparse_guard.mark_complete();
    }

    constexpr void _pop_page_to(const size_type page) noexcept {
        while (sparse_.size() != page) [[likely]] {
            sparse_.pop_back();
        }
    }

    NODISCARD constexpr auto _contains_impl(
        const key_type key, const size_type page,
        const size_type offset) const noexcept -> bool {
        if (dense_.empty() || sparse_.size() <= page) {
            return false;
        }

        return sparse_[page] && dense_[sparse_[page]->at(offset)].first == key;
    }

    template <typename... Args>
    constexpr std::pair<iterator, bool> _emplace_one_at_back(
        const key_type key, const size_type page, const size_type offset,
        Args&&... args) {
        auto dense_guard = make_exception_guard([this] { dense_.pop_back(); });
        const auto index = dense_.size();
        dense_.emplace_back(
            std::piecewise_construct, std::forward_as_tuple(key),
            std::forward_as_tuple(std::forward<Args>(args)...));
        _set_index(page, offset, index);
        dense_guard.mark_complete();
        return std::make_pair(dense_.begin() + (dense_.size() - 1), true);
    }

    constexpr void _set_index(
        const size_type page, const size_type offset, const size_type index) {
        _check_page(page);
        sparse_[page]->at(offset) = index;
    }

    /**
     * @brief Erase element without check.
     * May cause error, but faster.
     * We could check before calling this.
     */
    constexpr iterator _erase_without_check_impl(
        const size_type page, const size_type offset) {
        auto& index       = sparse_[page]->at(offset);
        auto& back        = dense_.back();
        const auto backup = index;
        sparse_[_page_of(back.first)]->at(_offset_of(back.first)) = index;
        std::swap(dense_[index], back);
        dense_.pop_back();
        index = 0;
        return dense_.begin() + backup;
    }

    _dense_type dense_;
    _sparse_type sparse_;
};

template <
    typename Kty, typename Ty,
    typename Alloc = std::allocator<std::pair<Kty, Ty>>, size_t PageSize = 32UL,
    typename = std::enable_if_t<std::is_unsigned_v<Kty>>>
sparse_map(std::initializer_list<std::pair<Kty, Ty>>, Alloc = Alloc())
    -> sparse_map<Kty, Ty, PageSize, Alloc>;
template <
    typename Kty, typename Ty,
    typename Alloc = std::allocator<std::pair<Kty, Ty>>, size_t PageSize = 32UL,
    typename = std::enable_if_t<std::is_unsigned_v<Kty>>>
sparse_map(std::initializer_list<std::pair<const Kty, Ty>>, Alloc = Alloc())
    -> sparse_map<Kty, Ty, PageSize, Alloc>;
template <
    typename Kty, typename Ty,
    typename Alloc = std::allocator<std::pair<Kty, Ty>>, size_t PageSize = 32UL,
    typename = std::enable_if_t<std::is_unsigned_v<Kty>>>
sparse_map(std::initializer_list<compressed_pair<Kty, Ty>>, Alloc = Alloc())
    -> sparse_map<Kty, Ty, PageSize, Alloc>;
template <
    typename Kty, typename Ty,
    typename Alloc = std::allocator<std::pair<Kty, Ty>>, size_t PageSize = 32UL,
    typename = std::enable_if_t<std::is_unsigned_v<Kty>>>
sparse_map(
    std::initializer_list<compressed_pair<const Kty, Ty>>, Alloc = Alloc())
    -> sparse_map<Kty, Ty, PageSize, Alloc>;

namespace pmr {

template <std::unsigned_integral Kty, typename Ty, size_t PageSize = 32UL>
using sparse_map =
    sparse_map<Kty, Ty, PageSize, std::pmr::polymorphic_allocator<>>;

} // namespace pmr

} // namespace neutron

/*! @cond TURN_OFF_DOXYGEN */
namespace std {

template <unsigned_integral Kty, typename Ty, typename Alloc, size_t PageSize>
struct uses_allocator<neutron::sparse_map<Kty, Ty, PageSize, Alloc>, Alloc> :
    std::true_type {};

} // namespace std
/*! @endcond */
