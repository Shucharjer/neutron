#pragma once
#include <algorithm>
#include <compare>
#include <iterator>
#include <memory>
#include <type_traits>
#include <version>
#include "neutron/detail/concepts/allocator.hpp"
#if defined(__cpp_lib_flat_map) && __cpp_lib_flat_map >= 202202L
    #include <flat_map>
#else
    #include <functional>
    #include "neutron/detail/macros.hpp"
#endif

namespace neutron {

#if defined(__cpp_lib_flat_map) && __cpp_lib_flat_map >= 202202L

using std::flat_map;

#else

template <
    typename Kty, typename Ty, typename Compare = std::less<Kty>,
    typename KtyContainer = std::vector<Kty>,
    typename Container    = std::vector<Ty>>
class flat_map {
public:
    using key_type              = Kty;
    using mapped_type           = Ty;
    using value_type            = std::pair<const Kty, Ty>;
    using key_container_type    = KtyContainer;
    using mapped_container_type = Container;
    using size_type             = typename key_container_type::size_type;
    using difference_type       = typename key_container_type::difference_type;
    using key_compare           = Compare;
    using reference             = std::pair<const Kty&, Ty&>;
    using const_reference       = std::pair<const Kty&, const Ty&>;
    using allocator_type =
        rebind_alloc_t<typename KtyContainer::allocator_type, value_type>;
    using alloc_traits = std::allocator_traits<allocator_type>;

    template <bool IsConst>
    class _iterator {
        friend class _iterator<!IsConst>;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = typename flat_map::value_type;
        using difference_type   = typename flat_map::difference_type;
        using reference         = std::conditional_t<
                    IsConst, typename flat_map::const_reference,
                    typename flat_map::reference>;

        constexpr _iterator() noexcept = default;

        constexpr _iterator(const Kty* key, Ty* value) noexcept
            : key_(key), value_(value) {}

        constexpr reference operator*() noexcept {
            return reference{ *key_, *value_ };
        }

        constexpr _iterator& operator++() noexcept {
            ++key_;
            ++value_;
            return *this;
        }

        constexpr _iterator operator++(int) noexcept {
            _iterator temp = *this;
            this->operator++();
            return temp;
        }

        constexpr _iterator& operator--() noexcept {
            --key_;
            --value_;
            return *this;
        }

        constexpr _iterator operator--(int) noexcept {
            _iterator temp = *this;
            this->operator--();
            return temp;
        }

        constexpr _iterator& operator+=(difference_type n) noexcept {
            key_ += n;
            value_ += n;
            return *this;
        }

        constexpr _iterator& operator-=(difference_type n) noexcept {
            key_ -= n;
            value_ -= n;
            return *this;
        }

        constexpr _iterator operator+(difference_type n) const noexcept {
            return _iterator(key_ + n, value_ + n);
        }

        constexpr _iterator operator-(difference_type n) const noexcept {
            return _iterator(key_ - n, value_ - n);
        }

        constexpr difference_type
            operator-(const _iterator& that) const noexcept {
            return key_ - that.key_;
        }

        constexpr bool operator==(const _iterator& that) const noexcept {
            return key_ == that.key_;
        }

        constexpr bool operator!=(const _iterator& that) const noexcept {
            return key_ != that.key_;
        }

        constexpr std::strong_ordering
            operator<=>(const _iterator& that) const noexcept {
            return key_ <=> that.key_;
        }

    private:
        const Kty* key_;
        Ty* value_;
    };

    using iterator       = _iterator<false>;
    using const_iterator = _iterator<true>;

    template <typename Comp = Compare, typename Al = allocator_type>
    constexpr flat_map(const Comp& compare = {}, const Al& alloc = {})
        : keys_(alloc), values_(alloc), compare_(compare) {}

    template <typename Al = allocator_type>
    constexpr flat_map(const Al& alloc = {})
        : keys_(alloc), values_(alloc), compare_() {}

    template <typename... Args>
    constexpr auto emplace(Args&&... args);

    template <typename... Args>
    constexpr auto try_emplace(const Kty& key, Args&&... args);

    constexpr iterator erase(const_iterator where);

    constexpr auto erase(const Kty& key);

    constexpr iterator find(const Kty& key) noexcept {}

    constexpr const_iterator find(const Kty& key) const noexcept;

    constexpr mapped_type& at(const Kty& key) noexcept;

    constexpr const mapped_type& at(const Kty& key) const noexcept;

    constexpr mapped_type& operator[](const Kty& key);

    constexpr mapped_type& operator[](Kty&& key);

    constexpr size_type capacity() const noexcept {
        return std::min(keys_.capacity(), values_.capacity());
    }

    constexpr void clear() noexcept {
        keys_.clear();
        values_.clear();
    }

    constexpr void shrink_to_fit() {
        keys_.shrink_to_fit();
        values_.shrink_to_fit();
    }

    constexpr void reserve(size_type n) {
        keys_.reserve(n);
        values_.reserve(n);
    }

    ATOM_NODISCARD constexpr bool empty() const noexcept {
        return keys_.empty();
    }

    ATOM_NODISCARD constexpr size_type size() const noexcept {
        return keys_.size();
    }

    ATOM_NODISCARD constexpr size_type max_size() const noexcept {
        return keys_.max_size();
    }

    ATOM_NODISCARD constexpr bool contains(const Kty& key) const noexcept {
        return std::binary_search(keys_.begin(), keys_.end(), key);
    }

private:
    KtyContainer keys_;
    Container values_;
    ATOM_NO_UNIQUE_ADDR Compare compare_;
};

#endif

namespace pmr {

template <typename Kty, typename Ty, typename Compare = std::less<Kty>>
using flat_map =
    flat_map<Kty, Ty, Compare, std::pmr::vector<Kty>, std::pmr::vector<Ty>>;

} // namespace pmr

} // namespace neutron
