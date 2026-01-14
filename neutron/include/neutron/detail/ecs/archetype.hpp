// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <memory_resource>
#include <new>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "neutron/detail/ecs/component.hpp"
#include "neutron/detail/ecs/entity.hpp"
#include "neutron/detail/ecs/fwd.hpp"
#include "neutron/detail/utility/spreader.hpp"
#include "neutron/shift_map.hpp"
#include "neutron/type_hash.hpp"

namespace neutron {

template <component... Components>
class view;

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
// NOLINTBEGIN(modernize-avoid-c-arrays)

namespace _view {

template <component... Components>
class view;

template <component... Components>
class eview {
    friend class view<Components...>;

public:
    class iterator {
        friend class eview<Components...>;

        constexpr iterator(
            entity_t* entity,
            const std::tuple<std::remove_cvref_t<Components>*...>&
                storage) noexcept
            : entity_(entity), storage_(storage) {}

    public:
        using value_type      = std::tuple<entity_t, Components...>;
        using reference       = value_type; // As components could be reference.
        using size_type       = size_t;
        using difference_type = ptrdiff_t;
        using iterator_concept = std::contiguous_iterator_tag;

        constexpr iterator(const iterator& that) noexcept            = default;
        constexpr iterator(iterator&& that) noexcept                 = default;
        constexpr iterator& operator=(const iterator& that) noexcept = default;
        constexpr iterator& operator=(iterator&& that) noexcept      = default;
        constexpr ~iterator() noexcept                               = default;

        constexpr auto operator*() const noexcept -> value_type {
            return std::tuple_cat(
                std::tuple(*entity_),
                std::apply(
                    [](auto*... ptrs) {
                        return std::tuple<Components...>((*ptrs)...);
                    },
                    storage_));
        }

        // input_iterator

        constexpr auto operator++() noexcept -> iterator& {
            ++entity_;
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (++std::get<Is>(storage_), ...);
            }(std::index_sequence_for<Components...>());
            return *this;
        }
        constexpr auto operator++(int) noexcept -> iterator {
            iterator temp = *this;
            ++entity_;
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (++std::get<Is>(storage_), ...);
            }(std::index_sequence_for<Components...>());
            return temp;
        }

        constexpr std::strong_ordering
            operator<=>(const iterator& that) const noexcept {
            return storage_ <=> that.storage_;
        }

        constexpr bool operator==(const iterator& that) const noexcept {
            return storage_ == that.storage_;
        }

        constexpr bool operator!=(const iterator& that) const noexcept {
            return storage_ != that.storage_;
        }

        // bidirectional

        constexpr auto operator--() noexcept -> iterator& {
            --entity_;
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (--std::get<Is>(storage_), ...);
            }(std::index_sequence_for<Components...>());
            return *this;
        }
        constexpr auto operator--(int) noexcept -> iterator {
            iterator temp = *this;
            --entity_;
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (--std::get<Is>(storage_), ...);
            }(std::index_sequence_for<Components...>());
            return temp;
        }

        // random access

        constexpr auto operator+=(ptrdiff_t diff) noexcept -> iterator& {
            entity_ += diff;
            [this, diff]<size_t... Is>(std::index_sequence<Is...>) {
                ((std::get<Is>(storage_) += diff), ...);
            }(std::index_sequence_for<Components...>());
            return *this;
        }
        constexpr auto operator+(ptrdiff_t diff) const noexcept -> iterator {
            iterator temp = *this;
            temp += diff;
            return temp;
        }
        constexpr auto operator-=(ptrdiff_t diff) noexcept -> iterator& {
            entity_ -= diff;
            [this, diff]<size_t... Is>(std::index_sequence<Is...>) {
                ((std::get<Is>(storage_) -= diff), ...);
            }(std::index_sequence_for<Components...>());
            return *this;
        }
        constexpr auto operator-(ptrdiff_t diff) noexcept -> iterator {
            iterator temp = *this;
            temp -= diff;
            return temp;
        }

        constexpr ptrdiff_t operator-(const iterator& that) const noexcept {
            return entity_ - that.entity_;
        }

        // support range

        // satisfy sentinel_for.semiregular
        // satisfy semiregular.default_constructible
        constexpr iterator() noexcept = default;

    private:
        entity_t* entity_{};
        std::tuple<std::remove_cvref_t<Components>*...> storage_;
    };
    using value_type      = std::tuple<entity_t, Components...>;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    // using iterator        = _view::iterator<Components...>;
    using _type_list = neutron::type_list<std::remove_cvref_t<Components>...>;
    using _sorted_list =
        neutron::sorted_type_t<neutron::sorted_list_t<_type_list>>;

    static constexpr auto indices = neutron::sort_index<_type_list>();

    static_assert(
        sizeof...(Components) > 0,
        "the view of components should contains at least one component");

    template <typename Archetype>
    eview(Archetype& arche) noexcept
        : size_(arche.size_), entities_(arche.index2entity_.data()) {
        std::array storage = arche.template get<Components...>();
        [this, &storage]<size_t... Is>(std::index_sequence<Is...>) {
            ((std::get<Is>(storage_) =
                  reinterpret_cast<std::remove_cvref_t<Components>*>(
                      storage[Is])),
             ...);
        }(std::index_sequence_for<Components...>());
    }

    constexpr auto begin() const noexcept {
        return iterator{ entities_, storage_ };
    }
    constexpr auto end() const noexcept {
        return iterator{ entities_, storage_ } + size_;
    }
    constexpr auto rbegin() const noexcept {
        return std::make_reverse_iterator(
            iterator{ entities_, storage_ } + size_);
    }
    constexpr auto rend() const noexcept {
        return std::make_reverse_iterator(iterator{ entities_, storage_ });
    }
    constexpr auto size() const noexcept { return size_; }
    constexpr auto empty() const noexcept { return size_ == 0; }

    ATOM_NODISCARD constexpr auto entities() const noexcept
        -> std::span<entity_t> {
        return std::span<entity_t>{ entities_, size_ };
    }

private:
    size_type size_;
    entity_t* entities_;
    std::tuple<std::remove_cvref_t<Components>*...> storage_;
};

template <component... Components>
class view {
public:
    class iterator {
        friend class view<Components...>;

        constexpr iterator(
            const std::tuple<std::remove_cvref_t<Components>*...>&
                storage) noexcept
            : storage_(storage) {}

    public:
        using value_type      = std::tuple<Components...>;
        using reference       = value_type; // As components could be reference.
        using size_type       = size_t;
        using difference_type = ptrdiff_t;
        using iterator_concept = std::contiguous_iterator_tag;

        constexpr iterator(const iterator& that) noexcept            = default;
        constexpr iterator(iterator&& that) noexcept                 = default;
        constexpr iterator& operator=(const iterator& that) noexcept = default;
        constexpr iterator& operator=(iterator&& that) noexcept      = default;
        constexpr ~iterator() noexcept                               = default;

        constexpr auto operator*() const noexcept -> value_type {
            return std::apply(
                [](auto*... ptrs) {
                    return std::tuple<Components...>((*ptrs)...);
                },
                storage_);
        }

        // input_iterator

        constexpr auto operator++() noexcept -> iterator& {
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (++std::get<Is>(storage_), ...);
            }(std::index_sequence_for<Components...>());
            return *this;
        }
        constexpr auto operator++(int) noexcept -> iterator {
            iterator temp = *this;
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (++std::get<Is>(storage_), ...);
            }(std::index_sequence_for<Components...>());
            return temp;
        }

        constexpr std::strong_ordering
            operator<=>(const iterator& that) const noexcept {
            return storage_ <=> that.storage_;
        }

        constexpr bool operator==(const iterator& that) const noexcept {
            return storage_ == that.storage_;
        }

        constexpr bool operator!=(const iterator& that) const noexcept {
            return storage_ != that.storage_;
        }

        // bidirectional

        constexpr auto operator--() noexcept -> iterator& {
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (--std::get<Is>(storage_), ...);
            }(std::index_sequence_for<Components...>());
            return *this;
        }
        constexpr auto operator--(int) noexcept -> iterator {
            iterator temp = *this;
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (--std::get<Is>(storage_), ...);
            }(std::index_sequence_for<Components...>());
            return temp;
        }

        // random access

        constexpr auto operator+=(ptrdiff_t diff) noexcept -> iterator& {
            [this, diff]<size_t... Is>(std::index_sequence<Is...>) {
                ((std::get<Is>(storage_) += diff), ...);
            }(std::index_sequence_for<Components...>());
            return *this;
        }
        constexpr auto operator+(ptrdiff_t diff) const noexcept -> iterator {
            iterator temp = *this;
            temp += diff;
            return temp;
        }
        constexpr auto operator-=(ptrdiff_t diff) noexcept -> iterator& {
            [this, diff]<size_t... Is>(std::index_sequence<Is...>) {
                ((std::get<Is>(storage_) -= diff), ...);
            }(std::index_sequence_for<Components...>());
            return *this;
        }
        constexpr auto operator-(ptrdiff_t diff) noexcept -> iterator {
            iterator temp = *this;
            temp -= diff;
            return temp;
        }

        constexpr ptrdiff_t operator-(const iterator& that) const noexcept {
            return std::get<0>(storage_) - std::get<0>(that.storage_);
        }

        // support range

        // satisfy sentinel_for.semiregular
        // satisfy semiregular.default_constructible
        constexpr iterator() noexcept = default;

    private:
        std::tuple<std::remove_cvref_t<Components>*...> storage_;
    };
    using value_type      = std::tuple<Components...>;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    // using iterator        = _view::iterator<Components...>;
    using _type_list = neutron::type_list<std::remove_cvref_t<Components>...>;
    using _sorted_list =
        neutron::sorted_type_t<neutron::sorted_list_t<_type_list>>;

    static constexpr auto indices = neutron::sort_index<_type_list>();

    static_assert(
        sizeof...(Components) > 0,
        "the view of components should contains at least one component");

    template <typename Archetype>
    requires requires(Archetype& archetype) { archetype.emplace(); }
    view(Archetype& arche) noexcept : size_(arche.size_) {
        std::array storage = arche.template get<Components...>();
        [this, &storage]<size_t... Is>(std::index_sequence<Is...>) {
            ((std::get<Is>(storage_) =
                  reinterpret_cast<std::remove_cvref_t<Components>*>(
                      storage[Is])),
             ...);
        }(std::index_sequence_for<Components...>());
    }

    view(const eview<Components...>& ev) noexcept
        : size_(ev.size_), storage_(ev.storage_) {}

    constexpr auto begin() const noexcept { return iterator{ storage_ }; }
    constexpr auto end() const noexcept { return iterator{ storage_ } + size_; }
    constexpr auto rbegin() const noexcept {
        return std::make_reverse_iterator(iterator{ storage_ } + size_);
    }
    constexpr auto rend() noexcept {
        return std::make_reverse_iterator(iterator{ storage_ });
    }
    constexpr auto size() const noexcept { return size_; }
    constexpr auto empty() const noexcept { return size_ == 0; }

private:
    size_type size_;
    std::tuple<std::remove_cvref_t<Components>*...> storage_;
};

template <component... Components>
view(const eview<Components...>&) -> view<Components...>;

template <typename>
struct _removed_empty;
template <template <typename...> typename Vw, component... Components>
struct _removed_empty<Vw<Components...>> {
    template <typename Ty>
    using predicate_type = std::negation<std::is_empty<Ty>>;
    using type = neutron::type_list_filt_t<predicate_type, Vw<Components...>>;
};

} // namespace _view

template <component... Components>
class eview : public _view::_removed_empty<_view::eview<Components...>>::type {
public:
    using _view_base = _view::_removed_empty<_view::eview<Components...>>::type;
    using value_type = typename _view_base::value_type;
    using reference  = value_type;
    using size_type  = typename _view_base::size_type;
    using iterator   = typename _view_base::iterator;
};

template <component... Components>
class view : public _view::_removed_empty<_view::view<Components...>>::type {
    template <template <typename...> typename Vw>
    using _remove_empty = _view::_removed_empty<Vw<Components...>>::type;

public:
    using _view_base  = _remove_empty<_view::view>;
    using _eview_base = _remove_empty<_view::eview>;
    using value_type  = typename _view_base::value_type;
    using reference   = value_type;
    using size_type   = typename _view_base::size_type;
    using iterator    = typename _view_base::iterator;

    using _view_base::_view_base;
    view(const eview<Components...>& ev) noexcept
        : _view_base(static_cast<const _eview_base&>(ev)) {}
    using _view_base::begin;
    using _view_base::rbegin;
    using _view_base::end;
    using _view_base::rend;
    using _view_base::size;
    using _view_base::empty;
};

/**
 * @brief Compile-time metadata descriptor for a component type.
 *
 * This structure encapsulates essential compile-time properties of a type, such
 * as trivial copyability, relocatability, alignment, and size. It is primarily
 * used by the `archetype` to manage heterogeneous component storage.
 */
struct basic_info {
    uint64_t trivially_copyable : 1;
    uint64_t trivially_relocatable : 1;
    uint64_t trivially_move_assignable : 1;
    uint64_t _reserve : 13;
    uint64_t align : 16;
    uint64_t size : 32;

    /**
     * @brief Constructs a `basic_info` instance for type `Ty` at compile time.
     * @tparam Ty The type to introspect.
     * @return A `consteval`-computed `basic_info` object describing `Ty`.
     */
    template <typename Ty>
    consteval static basic_info make() noexcept {
        return { .trivially_copyable    = std::is_trivially_copyable_v<Ty>,
                 .trivially_relocatable = neutron::trivially_relocatable<Ty>,
                 .trivially_move_assignable =
                     std::is_trivially_move_assignable_v<Ty>,
                 ._reserve = 0,
                 .align    = std::is_empty_v<Ty> ? 0 : alignof(Ty),
                 .size     = std::is_empty_v<Ty> ? 0 : sizeof(Ty) };
    }
};

static_assert(sizeof(basic_info) == sizeof(uint64_t));

/**
 * @brief Generates an array of @ref basic_info objects from a type list.
 * @tparam Tys Parameter pack of component types.
 * @param[in] list A `type_list<Tys...>` instance (used only for deduction).
 * @return A compile-time `std::array<basic_info, sizeof...(Tys)>`.
 */
template <typename... Tys>
consteval auto make_info(neutron::type_list<Tys...>) noexcept {
    return std::array{ basic_info::make<Tys>()... };
}

/**
 * @brief Convenience overload that sorts the input types before generating
 * metadata.

 * Sorting ensures deterministic layout and hash computation across translation
 * units.
 * @tparam Tys Unsorted component types.
 * @return A compile-time array of sorted `basic_info` descriptors.
 */
template <typename... Tys>
consteval auto make_info() noexcept {
    return make_info(
        neutron::sorted_type_t<
            neutron::sorted_list_t<neutron::type_list<Tys...>>>{});
}

template <component... Comp>
struct add_components_t {};
template <component... Comp>
struct remove_components_t {};

/**
 * @brief Represents a homogeneous collection of entities sharing the same set
 * of components.

 * An archetype stores components in Structure-of-Arrays (SoA) layout. Each
 * component type has its own contiguous buffer, enabling efficient iteration
 * and cache-friendly access.
 * @tparam Alloc Allocator type conforming to `std_simple_allocator` concept.
 */
template <std_simple_allocator Alloc>
class archetype : public archetype<rebind_alloc_t<Alloc, std::byte>> {};

template <std_simple_allocator Alloc>
requires std::same_as<typename Alloc::value_type, std::byte>
class archetype<Alloc> {

    template <component...>
    friend class _view::view;
    template <component...>
    friend class _view::eview;

    template <typename Ty>
    using _allocator_t = rebind_alloc_t<Alloc, Ty>;

    template <typename Ty>
    using _vector_t = std::vector<Ty, _allocator_t<Ty>>;

    static constexpr size_t default_alignment = 32;
    static constexpr size_t initial_capacity  = 64;

    /**
     * @brief Custom deleter for aligned memory allocated via `operator
     * new(size, align_val_t)`.
     */
    struct _buffer_deletor {
        std::align_val_t align;

        /**
         * @brief Deallocates memory with the stored alignment.
         * @param ptr Pointer to deallocate.
         */
        constexpr void operator()(std::byte* ptr) const noexcept {
            ::operator delete(ptr, align);
        }
    };

    /// Alias for a unique pointer managing aligned byte buffers.
    using _buffer_ptr = std::unique_ptr<std::byte[], _buffer_deletor>;

public:
    using size_type        = size_t;
    using difference_type  = ptrdiff_t;
    using _hash_type       = uint32_t;
    using _hash_vector     = _vector_t<_hash_type>;
    using _ctor_fn         = void (*)(void*, size_type);
    using _mov_ctor_fn     = void* (*)(void*, size_type, void*);
    using _mov_assign_fn   = void (*)(void*, void*);
    using _dtor_fn         = void (*)(void*, size_type) noexcept;
    using allocator_type   = _allocator_t<std::byte>;
    using allocator_traits = std::allocator_traits<allocator_type>;

    /**
     * @brief Constructs an archetype from a list of component types.
     *
     * The component list is automatically sorted to ensure consistent layout.
     * @tparam Components Component types to store in this archetype.
     * @param[in] tag     Tag type `neutron::immediately_t` for disambiguation.
     * @param[in] list    Type list of components.
     * @param[in] alloc   Allocator instance.
     */
    template <component... Components, typename Al = Alloc>
    requires(sizeof...(Components) != 0)
    ATOM_CONSTEXPR_SINCE_CXX23 archetype(
        neutron::immediately_t, neutron::type_list<Components...>,
        const Al& alloc = {})
        : hash_list_(
              std::initializer_list<uint32_t>{
                  neutron::hash_of<Components>()... },
              alloc),
          basic_info_({ basic_info::make<Components>()... }, alloc),
          constructors_(
              { [](void* ptr, size_t n) noexcept(
                    std::is_nothrow_default_constructible_v<Components>) {
                  auto* const first = static_cast<Components*>(ptr);
                  std::uninitialized_default_construct_n(first, n);
              }... },
              alloc),
          move_constructors_(
              { [](void* dst, size_type n, void* src) noexcept(
                    std::is_nothrow_move_constructible_v<Components> ||
                    std::is_nothrow_copy_constructible_v<Components>) -> void* {
                  auto* const psrc = static_cast<Components*>(src);
                  auto* const pdst = static_cast<Components*>(dst);
                  return static_cast<void*>(
                      neutron::uninitialized_move_if_noexcept_n(psrc, n, pdst));
              }... },
              alloc),
          move_assignments_(
              { [](void* dst, void* src) noexcept(
                    std::is_nothrow_move_assignable_v<Components>) {
                  (*static_cast<Components*>(dst) =
                       std::move(*static_cast<Components*>(src)));
              }... },
              alloc),
          destructors_(
              { [](void* ptr, size_type n) noexcept {
                  std::destroy_n(static_cast<Components*>(ptr), n);
              }... },
              alloc),
          storage_(sizeof...(Components), alloc), capacity_(initial_capacity),
          hash_(neutron::make_array_hash<neutron::type_list<Components...>>()),
          entity2index_(alloc), index2entity_(alloc) {
        using namespace neutron;
        [this]<size_t... Is>(std::index_sequence<Is...>) {
            (_set_storage<Is, Components...>(), ...);
        }(std::index_sequence_for<Components...>());
    }

    /**
     * @brief Convenience constructor accepting a `type_spreader`.
     *
     * Internally sorts the component types before delegation.
     */
    template <component... Components, typename Al = Alloc>
    requires(sizeof...(Components) != 0)
    constexpr archetype(
        neutron::type_spreader<Components...>, const Al& alloc = {})
        : archetype(
              neutron::immediately,
              neutron::sorted_type_t<
                  neutron::sorted_list_t<neutron::type_list<Components...>>>{},
              alloc) {}

    template <component... Components>
    archetype(const archetype& archetype, add_components_t<Components...>)
        : hash_list_(archetype.get_allocator()),
          basic_info_(archetype.get_allocator()),
          constructors_(archetype.get_allocator()),
          move_constructors_(archetype.get_allocator()),
          move_assignments_(archetype.get_allocator()),
          destructors_(archetype.get_allocator()),
          storage_(archetype.get_allocator()),
          entity2index_(archetype.get_allocator()),
          index2entity_(archetype.get_allocator())
    //   created_(archetype.get_allocator())
    {
        using namespace neutron;
        using sorted_list =
            sorted_type_t<sorted_list_t<type_list<Components...>>>;
        constexpr auto hash_array = make_hash_array<type_list<Components...>>();
        constexpr auto metainfo   = _get_metainfo(sorted_list{});
        constexpr auto newinfo    = std::get<0>(metainfo);

        const auto size = archetype.hash_list_.size();
        size_type i     = 0;
        size_type j     = 0;
        while (i < size && j < hash_array.size()) {
            if (archetype.hash_list_[i] < hash_array[j]) {
                hash_list_.push_back(archetype.hash_list_[i]);
                const basic_info info = archetype.basic_info_[i];
                basic_info_.push_back(info);
                constructors_.push_back(archetype.constructors_[i]);
                move_constructors_.push_back(archetype.move_constructors_[i]);
                move_assignments_.push_back(archetype.move_assignments_[i]);
                destructors_.push_back(archetype.destructors_[i]);
                const auto align = _get_align(info.align);
                auto* const ptr  = _get_ptr(info.size, align);
                storage_.emplace_back(ptr, _buffer_deletor{ align });
                ++i;
            } else {
                hash_list_.push_back(hash_array[j]);
                const basic_info info = newinfo[j];
                basic_info_.push_back(info);
                constructors_.push_back(std::get<1>(metainfo)[j]);
                move_constructors_.push_back(std::get<2>(metainfo)[j]);
                move_assignments_.push_back(std::get<3>(metainfo)[j]);
                destructors_.push_back(std::get<4>(metainfo)[j]);
                const auto align = _get_align(info.align);
                auto* const ptr  = _get_ptr(info.size, align);
                storage_.emplace_back(ptr, _buffer_deletor{ align });
                ++j;
            }
        }
        for (; i < size; ++i) {
            hash_list_.push_back(archetype.hash_list_[i]);
            const basic_info info = basic_info_[i];
            basic_info_.push_back(info);
            constructors_.push_back(archetype.constructors_[i]);
            move_constructors_.push_back(archetype.move_constructors_[i]);
            move_assignments_.push_back(archetype.move_assignments_[i]);
            destructors_.push_back(archetype.destructors_[i]);
            const auto align = _get_align(info.align);
            auto* const ptr  = _get_ptr(info.size, align);
            storage_.emplace_back(ptr, _buffer_deletor{ align });
        }
        for (; j < hash_array.size(); ++j) {
            hash_list_.push_back(hash_array[j]);
            const basic_info info = newinfo[j];
            basic_info_.push_back(info);
            constructors_.push_back(std::get<1>(metainfo)[j]);
            move_constructors_.push_back(std::get<2>(metainfo)[j]);
            move_assignments_.push_back(std::get<3>(metainfo)[j]);
            destructors_.push_back(std::get<4>(metainfo)[j]);
            const auto align = _get_align(info.align);
            auto* const ptr  = _get_ptr(info.size, align);
            storage_.emplace_back(ptr, _buffer_deletor{ align });
        }
        capacity_ = initial_capacity;
        hash_     = hash_combine(hash_list_);
    }

    template <component... Components, typename Al = Alloc>
    constexpr archetype(
        const archetype& archetype, remove_components_t<Components...>)
        : hash_list_(archetype.get_allocator()),
          basic_info_(archetype.get_allocator()),
          constructors_(archetype.get_allocator()),
          move_constructors_(archetype.get_allocator()),
          move_assignments_(archetype.get_allocator()),
          destructors_(archetype.get_allocator()),
          storage_(archetype.get_allocator()),
          entity2index_(archetype.get_allocator()),
          index2entity_(archetype.get_allocator()) {
        using namespace neutron;
        constexpr auto hash_array = make_hash_array<type_list<Components...>>();

        const auto size = archetype.hash_list_.size();
        size_t index    = 0;
        size_t offset   = 0;
        while (index != hash_array.size() && offset < size) {
            if (hash_array[index] != archetype.hash_list_[offset]) {
                // emplace
                hash_list_.push_back(archetype.hash_list_[offset]);
                const auto info = archetype.basic_info_[offset];
                basic_info_.push_back(info);
                constructors_.push_back(archetype.constructors_[offset]);
                move_constructors_.push_back(
                    archetype.move_constructors_[offset]);
                move_assignments_.push_back(
                    archetype.move_assignments_[offset]);
                destructors_.push_back(archetype.destructors_[offset]);
                const auto align = _get_align(info.align);
                auto* const ptr  = _get_ptr(info.size, align);
                storage_.emplace_back(ptr, _buffer_deletor{ align });
                ++offset;
            } else {
                ++index;
                ++offset;
            }
        }
        for (; offset < size; ++offset) {
            hash_list_.push_back(archetype.hash_list_[offset]);
            const auto info = archetype.basic_info_[offset];
            basic_info_.push_back(info);
            constructors_.push_back(archetype.constructors_[offset]);
            move_constructors_.push_back(archetype.move_constructors_[offset]);
            move_assignments_.push_back(archetype.move_assignments_[offset]);
            destructors_.push_back(archetype.destructors_[offset]);
            const auto align = _get_align(info.align);
            auto* const ptr  = _get_ptr(info.size, align);
            storage_.emplace_back(ptr, _buffer_deletor{ align });
        }
        capacity_ = initial_capacity;
        hash_     = hash_combine(hash_list_);
    }

    archetype(const archetype&)            = delete;
    archetype& operator=(const archetype&) = delete;

    /**
     * @brief Move constructor.
     *
     * Transfers ownership of all resources. Leaves the source in a valid but
     * unspecified state.
     */
    archetype(archetype&& that) noexcept
        : hash_list_(std::move(that.hash_list_)),
          basic_info_(std::move(that.basic_info_)),
          constructors_(std::move(that.constructors_)),
          move_constructors_(std::move(that.move_constructors_)),
          destructors_(std::move(that.destructors_)),
          storage_(std::move(that.storage_)),
          size_(std::exchange(that.size_, 0)),
          capacity_(std::exchange(that.capacity_, 0)),
          hash_(std::exchange(that.hash_, 0)),
          entity2index_(std::move(that.entity2index_)),
          index2entity_(std::move(that.index2entity_)) {}

    archetype& operator=(archetype&&) = delete;

    /**
     * @brief Destructor.
     *
     * Invokes destructors for all live components and deallocates storage.
     */
    ATOM_CONSTEXPR_SINCE_CXX23 ~archetype() noexcept {
        for (auto i = 0; i < hash_list_.size(); ++i) {
            const basic_info info = basic_info_[i];
            const auto size       = info.size;
            if (size == 0) { // empty component
                continue;
            }

            _dtor_fn dtor        = destructors_[i];
            std::byte* const ptr = storage_[i].get();
            destructors_[i](ptr, size_);
        }
        hash_list_.clear();
    }

    /**
     * @brief Checks whether the archetype contains the specified component(s).
     *
     * @tparam Args Component type(s) to query.
     * @return `true` if all queried types are present; otherwise `false`.
     */
    template <typename... Args>
    ATOM_NODISCARD constexpr bool has() const noexcept {
        if constexpr (sizeof...(Args) == 1U) {
            constexpr auto hash = neutron::hash_of<Args...>();
            return std::binary_search(
                this->hash_list_.begin(), this->hash_list_.end(), hash);
        } else {
            return (has<Args>() && ...);
        }
    }

    ATOM_NODISCARD constexpr auto emplace(entity_t entity) { _emplace(entity); }

    template <component... Components>
    ATOM_NODISCARD auto emplace(entity_t entity) {
        constexpr uint64_t combined_hash =
            neutron::make_array_hash<neutron::type_list<Components...>>();
        assert(combined_hash == hash_);

        using namespace neutron;
        using sorted_list  = sorted_list_t<type_list<Components...>>;
        using sorted_types = sorted_type_t<sorted_list>;

        _emplace(entity, sorted_types{});
    }

    template <component... Components>
    ATOM_NODISCARD constexpr auto
        emplace(entity_t entity, Components&&... components) {
        constexpr uint64_t combined_hash = neutron::make_array_hash<
            neutron::type_list<std::remove_cvref_t<Components>...>>();
        assert(combined_hash == hash_);

        _emplace(entity, std::forward<Components>(components)...);
    }

    constexpr void erase(entity_t entity) {
        const auto index      = entity2index_.at(entity);
        const auto last_index = size_ - 1;

        if (index != last_index) {
            for (uint32_t i = 0; i < hash_list_.size(); ++i) {
                const basic_info info = basic_info_[i];
                if (info.size == 0) {
                    continue;
                }

                auto* const data = storage_[i].get();
                auto* dst        = data + (info.size * index);
                auto* src        = data + (info.size * last_index);

                if (info.trivially_move_assignable) {
                    std::memcpy(dst, src, info.size);
                } else {
                    // requires component move assignable
                    move_assignments_[i](dst, src);
                    // requires component destructible
                    destructors_[i](src, 1);
                }
            }

            const auto last_entity     = index2entity_[last_index];
            index2entity_[index]       = last_entity;
            entity2index_[last_entity] = index;
        } else {
            for (uint32_t i = 0; i < hash_list_.size(); ++i) {
                const basic_info info = basic_info_[i];
                if (info.size == 0) {
                    continue;
                }

                auto* const data = storage_[i].get();
                destructors_[i](data + (info.size * index), 1);
            }
        }

        entity2index_.erase(entity);
        index2entity_.pop_back();
        --size_;
    }

    ATOM_NODISCARD constexpr size_type kinds() const noexcept {
        return hash_list_.size();
    }

    ATOM_NODISCARD constexpr size_type size() const noexcept { return size_; }

    ATOM_NODISCARD constexpr bool empty() const noexcept {
        return size_ == 0UL;
    }

    ATOM_NODISCARD constexpr size_type capacity() const noexcept {
        return capacity_;
    }

    ATOM_NODISCARD constexpr uint64_t hash() const noexcept { return hash_; }

    ATOM_NODISCARD constexpr auto hash_list() const noexcept
        -> const _vector_t<_hash_type>& {
        return hash_list_;
    }

    ATOM_NODISCARD _buffer_ptr* data() noexcept { return storage_.data(); }

    template <component... Components>
    ATOM_NODISCARD constexpr auto view() noexcept {
        return ::neutron::view<Components...>{ *this };
    }

    template <component... Components>
    ATOM_NODISCARD constexpr auto get()
        -> std::array<std::byte*, sizeof...(Components)> {
        using namespace neutron;
        using type_list   = type_list<std::remove_cvref_t<Components>...>;
        using sorted_list = sorted_list_t<type_list>;
        using sorted_type = sorted_type_t<sorted_list>;
        auto sorted       = _get_sorted(sorted_type{});
        return _apply_indices(sorted, type_list{});
    }

    ATOM_NODISCARD constexpr auto entities() noexcept {
        return entity2index_ | std::views::keys;
    }

    constexpr void reserve(size_type n) {
        entity2index_.reserve(n);
        index2entity_.reserve(n);
        if (capacity_ >= n) {
            return;
        }

        _relocate(n);
    }

    ATOM_NODISCARD constexpr auto clear() {
        const auto kinds = hash_list_.size();
        for (size_type i = 0; i < kinds; ++i) {
            const basic_info info = basic_info_[i];
            auto* const ptr       = storage_[i].get();
            destructors_[i](ptr, size_);
        }
        index2entity_.clear();
        entity2index_.clear();
        size_ = 0;
    }

    ATOM_NODISCARD constexpr auto get_allocator() const noexcept {
        return hash_list_.get_allocator();
    }

private:
    constexpr static std::align_val_t _get_align(size_t align) noexcept {
        return std::align_val_t{ (std::max<size_t>)(default_alignment, align) };
    }

    constexpr static std::byte*
        _get_ptr(size_t size, std::align_val_t align) noexcept {
        return static_cast<std::byte*>(
            ::operator new(size * initial_capacity, align));
    }

    template <size_t Index, component... SortedComponents>
    ATOM_CONSTEXPR_SINCE_CXX23 void _set_storage() {
        using namespace neutron;
        using type = type_list_element_t<Index, type_list<SortedComponents...>>;
        constexpr auto align = _get_align(alignof(type));

        if constexpr (std::is_empty_v<type>) {
            return;
        }

        _buffer_ptr& data = storage_[Index];

        data = _buffer_ptr{ _get_ptr(sizeof(type), align),
                            _buffer_deletor{ align } };
    }

    template <component... SortedComponents>
    consteval auto
        _get_metainfo(neutron::type_list<SortedComponents...>) noexcept {
        constexpr std::array basic_info   = make_info<SortedComponents...>();
        constexpr std::array constructors = {
            [](void* ptr, size_type n) noexcept(
                std::is_nothrow_default_constructible_v<SortedComponents>) {
                if constexpr (!std::is_empty_v<SortedComponents>) {
                    std::uninitialized_default_construct_n(
                        static_cast<SortedComponents*>(ptr), n);
                }
            }...
        };
        constexpr std::array move_constructors = {
            [](void* dst, size_type n, void* src) noexcept(
                std::is_nothrow_move_constructible_v<SortedComponents> ||
                std::is_nothrow_copy_constructible_v<SortedComponents>) {
                if constexpr (!std::is_empty_v<SortedComponents>) {
                    auto* const ofirst = static_cast<SortedComponents*>(dst);
                    auto* const ifirst = static_cast<SortedComponents*>(src);
                    neutron::uninitialized_move_if_noexcept_n(
                        ifirst, n, ofirst);
                }
            }...
        };
        constexpr std::array move_assignments = {
            [](void* dst, void* src) noexcept(
                std::is_nothrow_move_assignable_v<SortedComponents>) {
                if constexpr (!std::is_empty_v<SortedComponents>) {
                    *static_cast<SortedComponents*>(dst) =
                        std::move(*static_cast<SortedComponents*>(src));
                }
            }...
        };
        constexpr std::array destructors = { [](void* ptr,
                                                size_type n) noexcept {
            if constexpr (!std::is_empty_v<SortedComponents>) {
                auto* const first = static_cast<SortedComponents*>(ptr);
                std::destroy_n(first, n);
            }
        }... };
        return std::make_tuple(
            basic_info, constructors, move_constructors, move_assignments,
            destructors);
    }

    constexpr auto _emplace(entity_t entity) {
        const index_t index = size_;
        if (size_ == capacity_) [[unlikely]] {
            _relocate(capacity_ << 1);
        }
        _emplace_normally();
        entity2index_.try_emplace(entity, index);
        index2entity_.push_back(entity);
        ++size_;
    }

    constexpr auto _emplace_normally() {
        size_type idx = 0;
        auto guard    = neutron::make_exception_guard([this, &idx]() noexcept {
            for (auto i = idx; i >= 0; --i) {
                const basic_info info = basic_info_[i];
                if (info.size == 0) {
                    continue;
                }

                _buffer_ptr& data = storage_[i];
                auto* const ptr   = data.get();
                destructors_[i](ptr + (idx * info.size), 1);
            }
        });

        for (auto i = 0; i < hash_list_.size(); ++i) {
            const basic_info info = basic_info_[i];
            if (info.size == 0) {
                continue;
            }

            _buffer_ptr& data = storage_[i];
            auto* const ptr   = data.get();
            constructors_[i](ptr + (size_ * info.size), 1);
        }
    }

    auto _prepare_for_relocation(
        const size_type kinds, _vector_t<_buffer_ptr>& buffers,
        size_type capacity) {
        for (size_type i = 0; i < kinds; ++i) {
            const basic_info info = basic_info_[i];
            if (info.size == 0) {
                continue;
            }

            const auto align = _get_align(info.align);

            buffers[i] = _buffer_ptr{ _get_ptr(info.size, align),
                                      _buffer_deletor{ align } };
        }
    }

    auto
        _relocate_data(const size_type kinds, _vector_t<_buffer_ptr>& buffers) {
        size_type idx = kinds - 1;
        auto guard    = neutron::make_exception_guard(
            [this, kinds, &idx, &buffers]() noexcept {
                for (size_type i = idx; i < kinds; ++i) {
                    const basic_info info = basic_info_[i];
                    if (info.size == 0) {
                        continue;
                    }
                    destructors_[i](buffers[i].get(), size_);
                }
            });
        for (; idx >= 0; --idx) {
            const basic_info info = basic_info_[idx];
            if (info.size == 0) {
                continue;
            }

            void* const dst = buffers[idx].get();

            // NOTE: move_constructors_ uses uninitialized_move_if_noexcept_n
            move_constructors_[idx](
                buffers[idx].get(), size_, storage_[idx].get());
        }
        guard.mark_complete();
        for (size_type i = 0; i < kinds; ++i) {
            destructors_[i](storage_[i].get(), size_);
        }
        storage_ = std::move(buffers);
    }

    auto _relocate(size_type capacity) {
        const auto kinds = hash_list_.size();
        _vector_t<_buffer_ptr> buffers{ kinds, entity2index_.get_allocator() };
        _prepare_for_relocation(kinds, buffers, capacity);
        _relocate_data(kinds, buffers);
        capacity_ = capacity;
    }

    // relocation

    template <
        size_t Index, typename TypeList,
        typename Ty = neutron::type_list_element_t<Index, TypeList>>
    auto _prepare_for_relocation(
        _vector_t<_buffer_ptr>& buffers,
        size_type capacity) noexcept(std::is_empty_v<Ty>) {
        if constexpr (std::is_empty_v<Ty>) {
            return;
        }

        constexpr auto align = _get_align(alignof(Ty));

        auto* const ptr = static_cast<std::byte*>(
            ::operator new(sizeof(Ty) * capacity, align));
        buffers[Index] = _buffer_ptr{ ptr, _buffer_deletor{ align } };
    }

    template <
        size_t Index, typename TypeList,
        typename Ty = neutron::type_list_element_t<Index, TypeList>>
    requires neutron::nothrow_conditional_movable<Ty>
    auto _relocate_data(
        _vector_t<_buffer_ptr>& buffers,
        [[maybe_unused]] size_type& succ) noexcept {
        if constexpr (std::is_empty_v<Ty>) {
            return;
        }

        constexpr auto align = _get_align(alignof(Ty));
        constexpr auto alg   = static_cast<size_t>(align);

        _buffer_ptr& data = storage_[Index];
        auto* const src =
            std::assume_aligned<alg>(reinterpret_cast<Ty*>(data.get()));
        auto* const dst = std::assume_aligned<alg>(
            reinterpret_cast<Ty*>(buffers[Index].get()));
        if constexpr (neutron::trivially_relocatable<Ty>) {
            std::memcpy(dst, src, sizeof(Ty) * size_);
        } else {
            neutron::uninitialized_move_if_noexcept_n(
                std::assume_aligned<alg>(src), size_,
                std::assume_aligned<alg>(dst));
        }
        ++succ;
    }

    template <
        size_t Index, typename TypeList,
        typename Ty = neutron::type_list_element_t<Index, TypeList>>
    requires(!neutron::nothrow_conditional_movable<Ty>)
    auto _relocate_data(_vector_t<_buffer_ptr>& buffers, size_type& succ) {
        constexpr auto align     = _get_align(alignof(Ty));
        constexpr auto alignment = static_cast<size_t>(align);

        _buffer_ptr& data = storage_[Index];
        auto* const src   = reinterpret_cast<Ty*>(data.get());
        auto* const dst   = reinterpret_cast<Ty*>(buffers[Index].get());
        neutron::uninitialized_move_if_noexcept_n(
            std::assume_aligned<alignment>(src), size_,
            std::assume_aligned<alignment>(dst));
        ++succ;
    }

    template <
        size_t Index, typename TypeList,
        typename Ty = neutron::type_list_element_t<Index, TypeList>>
    auto _clean_for_relocation(
        _vector_t<_buffer_ptr>& buffers, size_type succ) noexcept {
        if constexpr (std::is_empty_v<Ty>) {
            return;
        }

        constexpr auto align = _get_align(alignof(Ty));
        constexpr auto alg   = static_cast<size_t>(align);

        if (Index < succ) {
            _buffer_ptr& data = buffers[Index];
            auto* const ptr =
                std::assume_aligned<alg>(reinterpret_cast<Ty*>(data.get()));
            std::destroy_n(ptr, size_);
        }
    }

    template <
        size_t Index, typename TypeList,
        typename Ty = neutron::type_list_element_t<Index, TypeList>>
    auto _apply_relocation() noexcept {
        auto* const ptr = reinterpret_cast<Ty*>(storage_[Index].get());
        std::destroy_n(ptr, size_);
    }

    template <component... Components>
    auto _apply_relocation(_vector_t<_buffer_ptr>& buffers) noexcept {
        using tlist = neutron::type_list<Components...>;
        [this]<size_t... Is>(std::index_sequence<Is...>) {
            (_apply_relocation<Is, tlist>(), ...);
        }(std::index_sequence_for<Components...>());
        storage_ = std::move(buffers);
    }

    template <component... Components>
    requires(neutron::nothrow_conditional_movable<Components> && ...)
    auto _relocate(size_type capacity) {
        using tlist          = neutron::type_list<Components...>;
        constexpr auto count = sizeof...(Components);
        [this, capacity]<size_t... Is>(std::index_sequence<Is...>) {
            _vector_t<_buffer_ptr> buffers(count, storage_.get_allocator());
            (_prepare_for_relocation<Is, tlist>(buffers, capacity), ...);
            [[maybe_unused]] size_type succ = 0;
            (_relocate_data<Is, tlist>(buffers, succ), ...);
            (_apply_relocation<Is, tlist>(), ...);
            storage_ = std::move(buffers);
        }(std::index_sequence_for<Components...>());
        capacity_ = capacity;
    }

    template <component... Components>
    requires(!(neutron::nothrow_conditional_movable<Components> && ...))
    auto _relocate(size_type capacity) {
        using tlist        = neutron::type_list<Components...>;
        constexpr auto num = sizeof...(Components);

        [this, capacity]<size_t... Is>(std::index_sequence<Is...>) {
            _vector_t<_buffer_ptr> buffers(num, storage_.get_allocator());
            (_prepare_for_relocation<Is, tlist>(buffers), ...);
            size_type succ = 0;
            auto guard     = neutron::make_exception_guard([this, &succ] {
                if (succ != num) [[unlikely]] {
                    ((_clean_for_relocation<Is, tlist>(buffers, succ)), ...);
                }
            });
            (_relocate_data<Is, tlist>(buffers, succ), ...);
            (_apply_relocation<Is, tlist>(), ...);
            storage_ = std::move(buffers);
            guard.mark_complete();
        }(std::index_sequence_for<Components...>());
        capacity_ = capacity;
    }

    // emplace<...>();

    template <
        size_t Index, typename TypeList,
        typename Ty = neutron::type_list_element_t<Index, TypeList>>
    requires std::is_nothrow_default_constructible_v<Ty>
    constexpr void _emplace_one_normally_noexcept() noexcept {
        if constexpr (std::is_empty_v<Ty>) {
            return;
        }

        _buffer_ptr& data = storage_[Index];
        auto* const ptr   = data.get();
        ::new (ptr + (sizeof(Ty) * size_)) Ty();
    }

    template <component... Components>
    requires(std::is_nothrow_default_constructible_v<Components> && ...)
    constexpr void _emplace_normally() noexcept {
        [this]<size_t... Is>(std::index_sequence<Is...>) {
            (_emplace_one_normally_noexcept<
                 Is, neutron::type_list<Components...>>(),
             ...);
        }(std::index_sequence_for<Components...>());
    }

    template <
        size_t Index, typename TypeList,
        typename Ty = neutron::type_list_element_t<Index, TypeList>>
    constexpr void _emplace_one_normally(size_t& succ) noexcept(
        std::is_nothrow_default_constructible_v<Ty>) {
        if constexpr (std::is_empty_v<Ty>) {
            return;
        }

        _buffer_ptr& data = storage_[Index];
        auto* const ptr   = data.get();
        ::new (ptr + (sizeof(Ty) * size_)) Ty();
        ++succ;
    }

    template <
        size_t Index, typename TypeList,
        typename Ty = neutron::type_list_element_t<Index, TypeList>>
    void _clean_for_emplace_one(size_t succ) noexcept {
        if (Index < succ) {
            _buffer_ptr& data = storage_[Index];
            auto* const ptr   = data.get();
            reinterpret_cast<Ty*>(ptr + (sizeof(Ty) * size_))->~Ty();
        }
    }

    template <component... Components>
    requires(!(std::is_nothrow_default_constructible_v<Components> && ...))
    void _emplace_normally() {
        using tlist          = neutron::type_list<Components...>;
        constexpr size_t num = sizeof...(Components);
        size_t succ          = 0;
        auto guard           = neutron::make_exception_guard([this, &succ] {
            if (succ != num) [[unlikely]] {
                [this, succ]<size_t... Is>(std::index_sequence<Is...>) {
                    (_clean_for_emplace_one<Is, tlist>(succ), ...);
                }(std::index_sequence_for<Components...>());
            }
        });
        [this, &succ]<size_t... Is>(std::index_sequence<Is...>) {
            (_emplace_one_normally<Is, tlist>(succ), ...);
        }(std::index_sequence_for<Components...>());
        guard.mark_complete();
    }

    template <component... Components>
    auto _emplace(
        entity_t entity, [[maybe_unused]] neutron::type_list<Components...>) {
        const index_t index = size_;
        if (size_ == capacity_) [[unlikely]] {
            _relocate<Components...>(capacity_ << 1);
        }
        _emplace_normally<Components...>();
        entity2index_.try_emplace(entity, index);
        index2entity_.push_back(entity);
        ++size_;
    }

    // emplace(...);

    template <
        size_t Index, typename TypeList, typename Tup,
        typename Ty = neutron::type_list_element_t<Index, TypeList>>
    requires std::is_nothrow_constructible_v<
        Ty, neutron::type_list_element_t<neutron::_rmcvref_first<Ty, Tup>, Tup>>
    constexpr void _emplace_one_val_normally_noexcept(Tup&& tup) noexcept {
        if constexpr (std::is_empty_v<Ty>) {
            return;
        }

        _buffer_ptr& data = storage_[Index];
        auto* const ptr   = data.get() + (sizeof(Ty) * size_);
        ::new (ptr) Ty(neutron::rmcvref_first<Ty>(std::forward<Tup>(tup)));
    }

    template <component... SortedComponents, typename Tup>
    requires(
        std::is_nothrow_constructible_v<
            SortedComponents,
            neutron::type_list_element_t<
                neutron::_rmcvref_first<SortedComponents, Tup>, Tup>> &&
        ...)
    constexpr void _emplace_vals_normally(
        [[maybe_unused]] neutron::type_list<SortedComponents...>,
        Tup&& tup) noexcept {
        using tlist = neutron::type_list<SortedComponents...>;
        [this, &tup]<size_t... Is>(std::index_sequence<Is...>) {
            (_emplace_one_val_normally_noexcept<Is, tlist>(
                 std::forward<Tup>(tup)),
             ...);
        }(std::index_sequence_for<SortedComponents...>());
    }

    template <
        size_t Index, typename TypeList, typename Tup,
        typename Ty = neutron::type_list_element_t<Index, TypeList>>
    requires std::is_nothrow_constructible_v<
        Ty, neutron::type_list_element_t<neutron::_rmcvref_first<Ty, Tup>, Tup>>
    constexpr void
        _emplace_one_val_normally(Tup&& tup, size_type& succ) noexcept {
        if constexpr (std::is_empty_v<Ty>) {
            return;
        }

        _buffer_ptr& data = storage_[Index];
        auto* const ptr   = data.get() + (sizeof(Ty) * size_);
        ::new (ptr) Ty(neutron::rmcvref_first<Ty>(std::forward<Tup>(tup)));
        ++succ;
    }

    template <
        size_t Index, typename TypeList, typename Tup,
        typename Ty = neutron::type_list_element_t<Index, TypeList>>
    requires(!std::is_nothrow_constructible_v<
             Ty, neutron::type_list_element_t<
                     neutron::_rmcvref_first<Ty, Tup>, Tup>>)
    constexpr void _emplace_one_val_normally(Tup&& tup, size_type& succ) {
        if constexpr (std::is_empty_v<Ty>) {
            return;
        }

        _buffer_ptr& data = storage_[Index];
        auto* const ptr   = data.get() + (sizeof(Ty) * size_);
        ::new (ptr) Ty(neutron::rmcvref_first<Ty>(std::forward<Tup>(tup)));
        ++succ;
    }

    template <component... SortedComponents, typename Tup>
    constexpr void _emplace_vals_normally(
        [[maybe_unused]] neutron::type_list<SortedComponents...>, Tup&& tup)
    requires(
        !(std::is_nothrow_constructible_v<
              SortedComponents,
              neutron::type_list_element_t<
                  neutron::_rmcvref_first<SortedComponents, Tup>, Tup>> &&
          ...))
    {
        using tlist = neutron::type_list<SortedComponents...>;

        [this, &tup]<size_t... Is>(std::index_sequence<Is...>) {
            size_type succ = 0;
            auto guard     = neutron::make_exception_guard([this, &succ] {
                if (succ != sizeof...(SortedComponents)) {
                    (_clean_for_emplace_one<Is, tlist>(), ...);
                }
            });
            (_emplace_one_val_normally<Is, tlist>(std::forward<Tup>(tup)), ...);
        }(std::index_sequence_for<SortedComponents...>());
    }

    template <component... SortedComponents, typename Tup>
    constexpr void _emplace(
        entity_t entity,
        [[maybe_unused]] neutron::type_list<SortedComponents...> sorted,
        Tup&& components) {
        const auto index = size_;
        if (size_ == capacity_) [[unlikely]] {
            _relocate<SortedComponents...>(capacity_ << 1);
        }
        _emplace_vals_normally(sorted, std::forward<Tup>(components));
        entity2index_.try_emplace(entity, index);
        index2entity_.push_back(entity);
        ++size_;
    }

    template <component... Components>
    constexpr void _emplace(entity_t entity, Components&&... components) {
        using namespace neutron;
        using sorted = sorted_type_t<
            sorted_list_t<type_list<std::remove_cvref_t<Components>...>>>;
        _emplace(
            entity, sorted{},
            std::forward_as_tuple(std::forward<Components>(components)...));
    }

    // vw

    template <component... Components>
    constexpr auto _get_sorted(neutron::type_list<Components...>) const noexcept
        -> std::array<std::byte*, sizeof...(Components)> {
        using namespace neutron;
        using type_list = type_list<Components...>;
        return [this]<size_t... Is>(std::index_sequence<Is...>) {
            std::array<std::byte*, sizeof...(Components)> result;
            auto hint = hash_list_.begin();
            (_get_sorted<Is, type_list>(result, hint), ...);
            return result;
        }(std::index_sequence_for<Components...>());
    }

    template <size_t Index, typename TypeList>
    constexpr void _get_sorted(auto& result, auto& hint) const noexcept {
        using namespace neutron;
        using type = type_list_element_t<Index, TypeList>;

        hint = std::lower_bound(hint, hash_list_.end(), hash_of<type>());
        const auto index = std::distance(hash_list_.begin(), hint);
        result[Index]    = storage_[index].get();
    }

    template <typename TypeList>
    constexpr auto _apply_indices(
        const auto& sorted, [[maybe_unused]] TypeList) const noexcept {
        constexpr std::array indices = neutron::sort_index<TypeList>();
        std::remove_cvref_t<decltype(sorted)> result;
        for (size_t i = 0; i < indices.size(); ++i) {
            result[i] = sorted[indices[i]];
        }
        return result;
    }

    _vector_t<_hash_type> hash_list_;
    _vector_t<basic_info> basic_info_;
    _vector_t<_ctor_fn> constructors_;
    _vector_t<_mov_ctor_fn> move_constructors_;
    _vector_t<_mov_assign_fn> move_assignments_;
    _vector_t<_dtor_fn> destructors_;
    _vector_t<_buffer_ptr> storage_;
    size_type size_{};
    size_type capacity_{};
    uint64_t hash_{};
    neutron::shift_map<
        entity_t, index_t, 256UL, neutron::half_bits<entity_t>, Alloc>
        entity2index_;
    // _vector_t<bool> created_;
    _vector_t<entity_t> index2entity_;
};

// NOLINTEND(modernize-avoid-c-arrays)
// NOLINTEND(cppcoreguidelines-avoid-c-arrays)
// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

namespace pmr {

using archetype = archetype<std::pmr::polymorphic_allocator<>>;

}

} // namespace neutron

namespace std {

template <neutron::std_simple_allocator Alloc>
struct uses_allocator<neutron::archetype<Alloc>, Alloc> : std::true_type {};

} // namespace std
