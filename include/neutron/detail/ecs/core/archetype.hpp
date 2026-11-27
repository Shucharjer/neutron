// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <memory_resource> // IWYU pragma: keep, for std::pmr::polymorphic_allocator
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <version>
#include "neutron/detail/algorithm/branchless_lower_bound.hpp"
#include "neutron/detail/concepts/allocator.hpp"
#include "neutron/detail/ecs/concepts/component.hpp"
#include "neutron/detail/ecs/concepts/entity.hpp"
#include "neutron/detail/ecs/utility/anchor.hpp"
#include "neutron/detail/memory/allocate_guard.hpp"
#include "neutron/detail/memory/uninitialized_move_if_noexcept.hpp"
#include "neutron/detail/ranges/concepts.hpp"
#include "neutron/detail/reflection/hash.hpp"
#include "neutron/detail/reflection/type_traits.hpp"
#include "neutron/detail/tuple/rmcvref_first.hpp"
#include "neutron/detail/utility/macros.hpp"
#include "neutron/detail/utility/spreader.hpp"
#include "neutron/metafn.hpp"
#include "neutron/shift_map.hpp"
#include "neutron/small_vector.hpp"
#include "neutron/utility.hpp"

namespace neutron {

template <component...>
class slice;

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
// NOLINTBEGIN(modernize-avoid-c-arrays)

template <component... Comp>
struct add_components_t {};
template <component... Comp>
struct remove_components_t {};

namespace _archetype {

constexpr std::size_t _initial_capacity = 64U;

// Component columns come from the archetype's allocator rebound to the
// element type, so the allocator sees the natural alignment of the component.

template <unfancy_allocator Alloc>
ATOM_ATTRS(ATOM_NODISCARD)
ATOM_CONSTEXPR_SINCE_CXX26 auto _allocate(Alloc alloc, std::size_t count)
    -> std::allocator_traits<Alloc>::pointer {
    return std::allocator_traits<Alloc>::allocate(alloc, count);
}

template <unfancy_allocator Alloc>
ATOM_CONSTEXPR_SINCE_CXX26 void _deallocate(
    Alloc alloc, typename std::allocator_traits<Alloc>::pointer ptr,
    std::size_t count) noexcept {
    std::allocator_traits<Alloc>::deallocate(alloc, ptr, count);
}

/// @brief Whether a column can be relocated without risking an exception.
template <typename T>
constexpr bool _relocates_nothrow =
    trivially_relocatible<T> || std::is_nothrow_move_constructible_v<T>;

template <typename T>
ATOM_ATTRS(ATOM_NODISCARD, ATOM_ALWAYS_INLINE, ATOM_NODEBUG)
inline ATOM_CONSTEXPR_SINCE_CXX26 auto _column(void* buffer) noexcept -> T* {
    return static_cast<T*>(buffer);
}

// Typed relocation of every column. Columns whose relocation may throw are
// copied first, so a failure leaves the source columns untouched.

template <
    std::size_t Index, typename Components, typename Alloc, std::size_t Size>
ATOM_CONSTEXPR_SINCE_CXX26 void _allocate_column(
    const Alloc& alloc, std::array<void*, Size>& nbufs, std::size_t ncap) {
    using comp_t = type_list_element_t<Index, Components>;
    if constexpr (!std::is_empty_v<comp_t>) {
        nbufs[Index] = _allocate(rebind_alloc_t<Alloc, comp_t>{ alloc }, ncap);
    }
}

template <std::size_t Index, typename Components, std::size_t Size>
ATOM_CONSTEXPR_SINCE_CXX26 void _copy_column(
    std::span<void*, Size> bufs, const std::array<void*, Size>& nbufs,
    std::array<bool, Size>& copied, std::size_t size) {
    using comp_t = type_list_element_t<Index, Components>;
    if constexpr (!std::is_empty_v<comp_t> && !_relocates_nothrow<comp_t>) {
        uninitialized_move_if_noexcept_n(
            _column<comp_t>(bufs[Index]), size, _column<comp_t>(nbufs[Index]));
        copied[Index] = true;
    }
}

template <std::size_t Index, typename Components, std::size_t Size>
ATOM_CONSTEXPR_SINCE_CXX26 void _move_column(
    std::span<void*, Size> bufs, const std::array<void*, Size>& nbufs,
    std::size_t size) noexcept {
    using comp_t = type_list_element_t<Index, Components>;
    if constexpr (!std::is_empty_v<comp_t> && _relocates_nothrow<comp_t>) {
        auto* const src = _column<comp_t>(bufs[Index]);
        auto* const dst = _column<comp_t>(nbufs[Index]);
        if constexpr (trivially_relocatible<comp_t>) {
            if ATOM_CONST_EVALUATED {
                std::uninitialized_move_n(src, size, dst);
                std::destroy_n(src, size);
            } else {
                // Relocation ends the lifetime of the sources.
                std::memcpy(dst, src, sizeof(comp_t) * size);
            }
        } else {
            std::uninitialized_move_n(src, size, dst);
            std::destroy_n(src, size);
        }
    }
}

template <
    std::size_t Index, typename Components, typename Alloc, std::size_t Size>
ATOM_CONSTEXPR_SINCE_CXX26 void _discard_column(
    const Alloc& alloc, const std::array<void*, Size>& nbufs,
    const std::array<bool, Size>& copied, std::size_t size,
    std::size_t ncap) noexcept {
    using comp_t = type_list_element_t<Index, Components>;
    if constexpr (!std::is_empty_v<comp_t>) {
        if (nbufs[Index] == nullptr) {
            return;
        }
        auto* const column = _column<comp_t>(nbufs[Index]);
        if (copied[Index]) {
            std::destroy_n(column, size);
        }
        _deallocate(rebind_alloc_t<Alloc, comp_t>{ alloc }, column, ncap);
    }
}

template <
    std::size_t Index, typename Components, typename Alloc, std::size_t Size>
ATOM_CONSTEXPR_SINCE_CXX26 void _commit_column(
    const Alloc& alloc, std::span<void*, Size> bufs,
    const std::array<void*, Size>& nbufs, std::size_t size,
    std::size_t ocap) noexcept {
    using comp_t = type_list_element_t<Index, Components>;
    if constexpr (!std::is_empty_v<comp_t>) {
        auto* const column = _column<comp_t>(bufs[Index]);
        if constexpr (!_relocates_nothrow<comp_t>) {
            // Copied, so the sources are still alive.
            std::destroy_n(column, size);
        }
        _deallocate(rebind_alloc_t<Alloc, comp_t>{ alloc }, column, ocap);
        bufs[Index] = nbufs[Index];
    }
}

/**
 * @brief Moves `size` rows of every column into buffers of `ncap` rows and
 * releases the old buffers of `ocap` rows.
 *
 * Strong guarantee: if an allocation or a copy throws, the columns are
 * unchanged.
 */
template <
    component... Components, typename Alloc,
    std::size_t Size = sizeof...(Components)>
ATOM_CONSTEXPR_SINCE_CXX26 void _relocate(
    const Alloc& alloc, std::span<void*, Size> bufs, std::size_t size,
    std::size_t ocap, std::size_t ncap) {
    using list_t = hash_list_t<type_list<std::remove_cvref_t<Components>...>>;
    using iseq_t = std::make_index_sequence<Size>;

    std::array<void*, Size> nbufs{};
    std::array<bool, Size> copied{};
    auto guard = make_exception_guard([&]() noexcept {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (_discard_column<Is, list_t>(alloc, nbufs, copied, size, ncap),
             ...);
        }(iseq_t());
    });

    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (_allocate_column<Is, list_t>(alloc, nbufs, ncap), ...);
        (_copy_column<Is, list_t>(bufs, nbufs, copied, size), ...);
    }(iseq_t());
    guard.dismiss();

    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (_move_column<Is, list_t>(bufs, nbufs, size), ...);
        (_commit_column<Is, list_t>(alloc, bufs, nbufs, size, ocap), ...);
    }(iseq_t());
}

// Typed construction of one row; `succ` counts the kinds constructed so far.

template <std::size_t Index, typename Components, std::size_t Size>
ATOM_CONSTEXPR_SINCE_CXX26 bool _clean_for_emplace_one(
    std::span<void* const, Size> bufs, std::size_t row,
    std::size_t succ) noexcept {
    using comp_t = std::remove_cvref_t<type_list_element_t<Index, Components>>;

    if (Index >= succ) {
        return false;
    }

    if constexpr (!std::is_empty_v<comp_t>) {
        std::destroy_at(_column<comp_t>(bufs[Index]) + row);
    }

    return true;
}

template <std::size_t Index, typename Components, std::size_t Size>
ATOM_CONSTEXPR_SINCE_CXX26 void _emplace_one_normally_impl(
    std::span<void* const, Size> bufs, std::size_t row,
    std::size_t&
        succ) noexcept(std::
                           is_nothrow_default_constructible_v<
                               type_list_element_t<Index, Components>>) {
    using comp_t = type_list_element_t<Index, Components>;

    if constexpr (!std::is_empty_v<comp_t>) {
        std::construct_at(_column<comp_t>(bufs[Index]) + row);
    }

    ++succ;
}

template <component... Components>
ATOM_CONSTEXPR_SINCE_CXX26 void _emplace_one_normally(
    std::span<void* const, sizeof...(Components)> bufs,
    std::size_t
        row) noexcept((std::is_nothrow_default_constructible_v<Components> && ...)) {
    using list_t = hash_list_t<type_list<Components...>>;
    using iseq_t = std::index_sequence_for<Components...>;

    constexpr auto nothrow =
        (std::is_nothrow_default_constructible_v<Components> && ...);

    [[maybe_unused]] std::size_t succ = 0;
    [[maybe_unused]] auto guard =
        make_exception_guard<nothrow>([bufs, &succ, row]() noexcept {
            [bufs, row, succ]<std::size_t... Is>(std::index_sequence<Is...>) {
                (_clean_for_emplace_one<Is, list_t>(bufs, row, succ) && ...);
            }(iseq_t());
        });

    [bufs, row, &succ]<std::size_t... Is>(std::index_sequence<Is...>) {
        (_emplace_one_normally_impl<Is, list_t>(bufs, row, succ), ...);
    }(iseq_t());

    guard.dismiss();
}

template <
    std::size_t Index, typename Components, typename Tup, std::size_t Size>
ATOM_CONSTEXPR_SINCE_CXX26 void _emplace_one_normally_impl(
    std::span<void* const, Size> bufs, std::size_t row, std::size_t& succ,
    Tup&& vals) noexcept(std::
                             is_nothrow_constructible_v<
                                 std::remove_cvref_t<
                                     type_list_element_t<Index, Components>>,
                                 type_list_element_t<Index, Components>>) {
    using comp_t = std::remove_cvref_t<type_list_element_t<Index, Components>>;

    if constexpr (!std::is_empty_v<comp_t>) {
        std::construct_at(
            _column<comp_t>(bufs[Index]) + row,
            rmcvref_first<comp_t>(std::forward<Tup>(vals)));
    }

    ++succ;
}

template <component... Components, typename Tup>
ATOM_CONSTEXPR_SINCE_CXX26 void _emplace_one_normally(
    std::span<void* const, sizeof...(Components)> bufs, std::size_t row,
    Tup&&
        vals) noexcept((std::is_nothrow_constructible_v<std::decay_t<Components>, Components> && ...)) {
    using list_t = hash_list_t<type_list<std::remove_cvref_t<Components>...>>;
    using iseq_t = std::index_sequence_for<Components...>;

    constexpr bool nothrow =
        (std::is_nothrow_constructible_v<
             std::decay_t<Components>, Components> &&
         ...);

    [[maybe_unused]] std::size_t succ = 0;
    [[maybe_unused]] auto guard =
        make_exception_guard<nothrow>([bufs, &succ, row]() noexcept {
            [bufs, row, succ]<std::size_t... Is>(std::index_sequence<Is...>) {
                (_clean_for_emplace_one<Is, list_t>(bufs, row, succ) && ...);
            }(iseq_t());
        });

    [bufs, row,
     &succ]<std::size_t... Is>(std::index_sequence<Is...>, auto&& vals) {
        (_emplace_one_normally_impl<Is, list_t>(
             bufs, row, succ, std::forward<decltype(vals)>(vals)),
         ...);
    }(iseq_t(), std::forward<Tup>(vals));

    guard.dismiss();
}

template <component... Components>
ATOM_CONSTEXPR_SINCE_CXX26 void _emplace_one_normally(
    std::span<void* const, sizeof...(Components)> bufs, std::size_t row,
    Components&&... vals) noexcept((std::is_nothrow_constructible_v<std::decay_t<Components>, Components> && ...)) {
    _emplace_one_normally<Components...>(
        bufs, row, std::forward_as_tuple(std::forward<Components>(vals)...));
}

} // namespace _archetype

/**
 * @brief Represents a homogeneous collection of entities sharing the same set
 * of components.

 * An archetype stores components in Structure-of-Arrays (SoA) layout. Each
 * component type has its own contiguous buffer, enabling efficient iteration
 * and cache-friendly access.
 * @tparam Alloc Allocator type conforming to `std_simple_allocator` concept.
 */
template <unfancy_allocator Alloc = std::allocator<std::byte>>
class archetype;

// do not support this
template <unfancy_allocator Alloc>
requires excluding<typename std::allocator_traits<Alloc>::value_type, std::byte>
class archetype<Alloc>;

template <unfancy_byte_allocator Alloc>
class archetype<Alloc> {

    template <component...>
    friend class slice;
    template <unfancy_allocator>
    friend class _world_base::world_base;

    template <typename Ty>
    using _allocator_t = rebind_alloc_t<Alloc, Ty>;

    static constexpr size_t initial_capacity = _archetype::_initial_capacity;

public:
    using size_type        = size_t;
    using difference_type  = ptrdiff_t;
    using allocator_type   = Alloc;
    using allocator_traits = std::allocator_traits<allocator_type>;

private:
    using _hash_type = hash_t;

    // Type-erased operations of one component kind. Rows are addressed by
    // index, so no untyped pointer arithmetic is needed.
    using _alloc_fn   = void* (*)(const allocator_type& alloc, size_type n);
    using _dealloc_fn = void (*)(
        const allocator_type& alloc, void* column, size_type n) noexcept;
    using _ctor_fn     = void (*)(void* column, size_type first, size_type n);
    using _mov_ctor_fn = void (*)(
        void* src, size_type src_first, void* dst, size_type dst_first,
        size_type n);
    using _mov_assign_fn = void (*)(void* column, size_type dst, size_type src);
    using _dtor_fn =
        void (*)(void* column, size_type first, size_type n) noexcept;

    template <typename T>
    struct _kind {
        static ATOM_CONSTEXPR_SINCE_CXX26 void*
            allocate(const allocator_type& alloc, size_type n) {
            if constexpr (std::is_empty_v<T>) {
                return nullptr;
            } else {
                return _archetype::_allocate(_allocator_t<T>{ alloc }, n);
            }
        }

        static ATOM_CONSTEXPR_SINCE_CXX26 void deallocate(
            const allocator_type& alloc, void* column, size_type n) noexcept {
            if constexpr (!std::is_empty_v<T>) {
                _archetype::_deallocate(
                    _allocator_t<T>{ alloc }, _archetype::_column<T>(column),
                    n);
            }
        }

        static ATOM_CONSTEXPR_SINCE_CXX26 void construct(
            void* column, size_type first,
            size_type n) noexcept(std::is_nothrow_default_constructible_v<T>) {
            if constexpr (!std::is_empty_v<T>) {
                std::uninitialized_value_construct_n(
                    _archetype::_column<T>(column) + first, n);
            }
        }

        static ATOM_CONSTEXPR_SINCE_CXX26 void
            move_construct(void* src, size_type src_first, void* dst, size_type dst_first, size_type n) noexcept(
                std::is_nothrow_move_constructible_v<T> ||
                std::is_nothrow_copy_constructible_v<T>) {
            if constexpr (!std::is_empty_v<T>) {
                uninitialized_move_if_noexcept_n(
                    _archetype::_column<T>(src) + src_first, n,
                    _archetype::_column<T>(dst) + dst_first);
            }
        }

        static ATOM_CONSTEXPR_SINCE_CXX26 void move_assign(
            void* column, size_type dst,
            size_type src) noexcept(std::is_nothrow_move_assignable_v<T>) {
            if constexpr (!std::is_empty_v<T>) {
                auto* const base = _archetype::_column<T>(column);
                base[dst]        = std::move(base[src]);
            }
        }

        static ATOM_CONSTEXPR_SINCE_CXX26 void
            destroy(void* column, size_type first, size_type n) noexcept {
            if constexpr (!std::is_empty_v<T>) {
                std::destroy_n(_archetype::_column<T>(column) + first, n);
            }
        }
    };

    /// @brief Compile-time metadata of hash-sorted component kinds.
    template <component... Components>
    struct _kind_table {
        static constexpr std::size_t count = sizeof...(Components);

        static constexpr std::array<_hash_type, count> hashes{
            hash_of<Components>()...
        };
        static constexpr std::array<type_traits, count> traits{
            type_traits_of<Components>()...
        };
        static constexpr std::array<_alloc_fn, count> allocs{
            &_kind<Components>::allocate...
        };
        static constexpr std::array<_dealloc_fn, count> deallocs{
            &_kind<Components>::deallocate...
        };
        static constexpr std::array<_ctor_fn, count> ctors{
            &_kind<Components>::construct...
        };
        static constexpr std::array<_mov_ctor_fn, count> mov_ctors{
            &_kind<Components>::move_construct...
        };
        static constexpr std::array<_mov_assign_fn, count> mov_assigns{
            &_kind<Components>::move_assign...
        };
        static constexpr std::array<_dtor_fn, count> dtors{
            &_kind<Components>::destroy...
        };
    };

public:
    /**
     * @brief Constructs an archetype from a list of component types.
     *
     * @tparam Components Component types, already sorted by hash.
     * @param[in] tag     Tag type `neutron::immediately_t` for disambiguation.
     * @param[in] list    Type list of components.
     * @param[in] alloc   Allocator instance.
     */
    template <component... Components, typename Al = Alloc>
    requires(sizeof...(Components) != 0)
    ATOM_CONSTEXPR_SINCE_CXX26 archetype(
        immediately_t, type_list<Components...>, const Al& alloc = {});

    /**
     * @brief Convenience constructor accepting a `type_spreader`.
     *
     * Internally sorts the component types before delegation.
     */
    template <component... Components, typename Al = Alloc>
    requires(sizeof...(Components) != 0)
    ATOM_CONSTEXPR_SINCE_CXX26
        archetype(type_spreader<Components...>, const Al& alloc = {});

    /// @brief Constructs the archetype `that` would have with `Components`.
    template <component... Components>
    ATOM_CONSTEXPR_SINCE_CXX26
        archetype(const archetype& that, add_components_t<Components...>);

    /// @brief Constructs the archetype `that` would have without
    /// `Components`.
    template <component... Components>
    ATOM_CONSTEXPR_SINCE_CXX26
        archetype(const archetype& that, remove_components_t<Components...>);

    archetype(const archetype&)            = delete;
    archetype& operator=(const archetype&) = delete;

    ATOM_CONSTEXPR_SINCE_CXX20 archetype(archetype&& that) noexcept;

    archetype& operator=(archetype&&) = delete;

    /**
     * @brief Destroys every live component and releases all storage.
     */
    ATOM_CONSTEXPR_SINCE_CXX26 ~archetype() noexcept;

    /**
     * @brief Checks whether the archetype contains the specified
     * component(s).
     *
     * @tparam Args Component type(s) to query.
     * @return `true` if all queried types are present; otherwise `false`.
     */
    template <typename... Args>
    ATOM_ATTRS(ATOM_NODISCARD)
    constexpr auto has() const noexcept;

    ATOM_ATTRS(ATOM_NODISCARD)
    ATOM_CONSTEXPR_SINCE_CXX26 auto emplace(entity_t entity);

    template <component... Components>
    ATOM_ATTRS(ATOM_NODISCARD)
    ATOM_CONSTEXPR_SINCE_CXX26 auto emplace(entity_t entity);

    template <component... Components>
    ATOM_ATTRS(ATOM_NODISCARD)
    ATOM_CONSTEXPR_SINCE_CXX26 auto emplace(auto&& range)
    requires compatible_range<decltype(range), entity_t>;

    template <component... Components>
    ATOM_ATTRS(ATOM_NODISCARD)
    ATOM_CONSTEXPR_SINCE_CXX26
        auto emplace(entity_t entity, Components&&... components);

    template <component... Components>
    ATOM_ATTRS(ATOM_NODISCARD)
    ATOM_CONSTEXPR_SINCE_CXX26
        auto emplace(auto&& range, Components&&... components)
    requires compatible_range<decltype(range), entity_t>;

    ATOM_CONSTEXPR_SINCE_CXX26 void erase(entity_t entity);

    template <component... Components>
    requires(!std::is_empty_v<std::remove_cvref_t<Components>> && ...)
    ATOM_ATTRS(ATOM_NODISCARD)
    ATOM_CONSTEXPR_SINCE_CXX26 auto at(entity_t entity)
        -> std::tuple<Components...>;

    ATOM_ATTRS(ATOM_NODISCARD) constexpr size_type kinds() const noexcept {
        return kinds_;
    }

    ATOM_ATTRS(ATOM_NODISCARD)
    constexpr auto size() const noexcept -> const size_type&;

    ATOM_ATTRS(ATOM_NODISCARD) constexpr bool empty() const noexcept;

    ATOM_ATTRS(ATOM_NODISCARD)
    constexpr auto capacity() const noexcept -> size_type;

    ATOM_ATTRS(ATOM_NODISCARD) constexpr long_hash_t hash() const noexcept;

    /// @brief Hashes of the stored component kinds, sorted ascending.
    ATOM_ATTRS(ATOM_NODISCARD)
    constexpr auto hash_list() const noexcept -> std::span<const hash_t>;

    /// @brief Column buffers, one per kind; null for empty components.
    ATOM_ATTRS(ATOM_NODISCARD) constexpr void* const* data() const noexcept;

    template <component... Components>
    ATOM_ATTRS(ATOM_NODISCARD)
    constexpr auto view_buffers() noexcept;

    ATOM_ATTRS(ATOM_NODISCARD)
    constexpr auto entities() noexcept -> std::span<entity_t>;

    ATOM_CONSTEXPR_SINCE_CXX26 void reserve(size_type n);

    ATOM_CONSTEXPR_SINCE_CXX26 void clear();

    ATOM_ATTRS(ATOM_NODISCARD)
    constexpr auto get_allocator() const noexcept -> allocator_type;

    ATOM_CONSTEXPR_SINCE_CXX26 void
        transfer(entity_t entity, archetype& target);

    template <component... Components>
    ATOM_CONSTEXPR_SINCE_CXX26 void transfer(
        entity_t entity, archetype& target,
        [[maybe_unused]] add_components_t<Components...>);

    template <component... Added, component... Components>
    ATOM_CONSTEXPR_SINCE_CXX26 void transfer(
        entity_t entity, archetype& target,
        [[maybe_unused]] add_components_t<Added...>,
        Components&&... components);

private:
    // kinds

    /// @brief Allocates the metadata of `count` kinds; columns are null.
    ATOM_CONSTEXPR_SINCE_CXX26 void _allocate_kinds(size_type count) {
        auto guard = make_allocate_guard<
            _hash_type, type_traits, _alloc_fn, _dealloc_fn, _ctor_fn,
            _mov_ctor_fn, _mov_assign_fn, _dtor_fn, void*>(count, alloc_);
        apply_allocation(
            guard, hash_list_, comp_traits_, allocs_, deallocs_, ctors_,
            mov_ctors_, mov_assigns_, dtors_, storage_);
        kinds_ = count;
        for (size_type i = 0; i < count; ++i) {
            std::construct_at(storage_ + i, nullptr);
        }
    }

    template <typename Table>
    constexpr void _set_kind(size_type kind, size_type index) noexcept {
        std::construct_at(hash_list_ + kind, Table::hashes[index]);
        std::construct_at(comp_traits_ + kind, Table::traits[index]);
        std::construct_at(allocs_ + kind, Table::allocs[index]);
        std::construct_at(deallocs_ + kind, Table::deallocs[index]);
        std::construct_at(ctors_ + kind, Table::ctors[index]);
        std::construct_at(mov_ctors_ + kind, Table::mov_ctors[index]);
        std::construct_at(mov_assigns_ + kind, Table::mov_assigns[index]);
        std::construct_at(dtors_ + kind, Table::dtors[index]);
    }

    constexpr void _copy_kind(
        size_type kind, const archetype& that, size_type index) noexcept {
        std::construct_at(hash_list_ + kind, that.hash_list_[index]);
        std::construct_at(comp_traits_ + kind, that.comp_traits_[index]);
        std::construct_at(allocs_ + kind, that.allocs_[index]);
        std::construct_at(deallocs_ + kind, that.deallocs_[index]);
        std::construct_at(ctors_ + kind, that.ctors_[index]);
        std::construct_at(mov_ctors_ + kind, that.mov_ctors_[index]);
        std::construct_at(mov_assigns_ + kind, that.mov_assigns_[index]);
        std::construct_at(dtors_ + kind, that.dtors_[index]);
    }

    /// @brief Allocates a column of `capacity` rows for every kind.
    ATOM_CONSTEXPR_SINCE_CXX26 void _allocate_columns(size_type capacity) {
        for (size_type i = 0; i < kinds_; ++i) {
            storage_[i] = allocs_[i](alloc_, capacity);
        }
    }

    template <typename T>
    ATOM_CONSTEXPR_SINCE_CXX20 void _deallocate_meta(T* ptr) noexcept {
        _allocator_t<T> alloc{ alloc_ };
        std::allocator_traits<_allocator_t<T>>::deallocate(alloc, ptr, kinds_);
    }

    /// @brief Releases every column of `capacity` rows and the metadata.
    ATOM_CONSTEXPR_SINCE_CXX26 void _release(size_type capacity) noexcept {
        if (storage_ == nullptr) {
            return;
        }

        for (size_type i = 0; i < kinds_; ++i) {
            if (storage_[i] != nullptr) {
                deallocs_[i](alloc_, storage_[i], capacity);
            }
        }
        _deallocate_meta(hash_list_);
        _deallocate_meta(comp_traits_);
        _deallocate_meta(allocs_);
        _deallocate_meta(deallocs_);
        _deallocate_meta(ctors_);
        _deallocate_meta(mov_ctors_);
        _deallocate_meta(mov_assigns_);
        _deallocate_meta(dtors_);
        _deallocate_meta(storage_);
        storage_ = nullptr;
        kinds_   = 0;
    }

    template <typename T>
    ATOM_ATTRS(ATOM_NODISCARD)
    constexpr auto _has_impl() const noexcept {
        const _hash_type* it  = hash_list_;
        const _hash_type* end = hash_list_ + kinds_;
        return [it,
                end]<std::size_t... Is>(std::index_sequence<Is...>) mutable {
            return (
                (it = branchless_lower_bound(
                     it, end, hash_of<type_list_element_t<Is, T>>()),
                 it != end && *it == hash_of<type_list_element_t<Is, T>>()) &&
                ...);
        }(std::make_index_sequence<type_list_size_v<T>>());
    }

    ATOM_CONSTEXPR_SINCE_CXX26 void
        _erase_known_index(entity_t entity, size_type index) {
        const auto last_index = size_ - 1;

        if (index != last_index) {
            for (size_type i = 0; i < kinds_; ++i) {
                // Empty components have no column, so the traits are not
                // needed to skip them.
                if (storage_[i] == nullptr) {
                    continue;
                }
                // requires component move assignable
                mov_assigns_[i](storage_[i], index, last_index);
                dtors_[i](storage_[i], last_index, 1);
            }

            const auto last_entity     = index2entity_[last_index];
            index2entity_[index]       = last_entity;
            entity2index_[last_entity] = index;
        } else {
            for (size_type i = 0; i < kinds_; ++i) {
                dtors_[i](storage_[i], index, 1);
            }
        }

        entity2index_.erase(entity);
        index2entity_.pop_back();
        --size_;
    }

    // relocation

    /// @brief Typed relocation; `Components` must be this archetype's
    /// kinds.
    template <component... Components>
    ATOM_CONSTEXPR_SINCE_CXX26 void _relocate_as(size_type capacity) {
        _archetype::_relocate<Components...>(
            alloc_, std::span<void*, sizeof...(Components)>{ storage_, kinds_ },
            size_, capacity_, capacity);
        capacity_ = capacity;
    }

    using _buffers_t = small_vector<void*, 64, _allocator_t<void*>>;

    /**
     * @brief Type-erased relocation for archetypes built at runtime.
     *
     * Strong guarantee, like the typed version: kinds that may throw are
     * copied before any source is moved from.
     */
    ATOM_CONSTEXPR_SINCE_CXX26 void _relocate(size_type capacity) {
        if (relocate_fn_ != nullptr) {
            relocate_fn_(this, capacity);
            return;
        }

        // Kinds whose move may throw are copied, so they are relocated
        // first, while every source is still intact.
        auto copies = [this](size_type kind) noexcept {
            const type_traits info = comp_traits_[kind];
            return info.size != 0 && !info.is_trivially_relocatible &&
                   !info.is_nothrow_move_constructible;
        };

        _buffers_t nbufs(kinds_, nullptr, _allocator_t<void*>{ alloc_ });
        size_type copied = 0; // kinds [0, copied) have been processed
        auto guard       = make_exception_guard([&]() noexcept {
            for (size_type i = 0; i < kinds_; ++i) {
                if (nbufs[i] == nullptr) {
                    continue;
                }
                if (i < copied && copies(i)) {
                    dtors_[i](nbufs[i], 0, size_);
                }
                deallocs_[i](alloc_, nbufs[i], capacity);
            }
        });

        for (size_type i = 0; i < kinds_; ++i) {
            nbufs[i] = allocs_[i](alloc_, capacity);
        }
        for (; copied < kinds_; ++copied) {
            if (copies(copied)) {
                mov_ctors_[copied](
                    storage_[copied], 0, nbufs[copied], 0, size_);
            }
        }
        guard.dismiss();

        for (size_type i = 0; i < kinds_; ++i) {
            const type_traits info = comp_traits_[i];
            if (info.size == 0) {
                continue;
            }
            if (copies(i)) {
                dtors_[i](storage_[i], 0, size_);
            } else if (info.is_trivially_relocatible) {
                if ATOM_CONST_EVALUATED {
                    mov_ctors_[i](storage_[i], 0, nbufs[i], 0, size_);
                    dtors_[i](storage_[i], 0, size_);
                } else {
                    // Relocation ends the lifetime of the sources.
                    std::memcpy(nbufs[i], storage_[i], info.size * size_);
                }
            } else {
                mov_ctors_[i](storage_[i], 0, nbufs[i], 0, size_);
                dtors_[i](storage_[i], 0, size_);
            }
            deallocs_[i](alloc_, storage_[i], capacity_);
            storage_[i] = nbufs[i];
        }
        capacity_ = capacity;
    }

    ATOM_CONSTEXPR_SINCE_CXX26 void _ensure_capacity_for_one() {
        const auto next_size = size_ + 1;
        size_type reserve_to = capacity_;
        if (next_size > capacity_) [[unlikely]] {
            const auto grow_to =
                (std::max)(next_size,
                           capacity_ == 0 ? initial_capacity : capacity_ << 1);
            _relocate(grow_to);
            reserve_to = grow_to;
        } else if (reserve_to < next_size) {
            reserve_to = next_size;
        }

        if (entity2index_.capacity() < reserve_to) {
            entity2index_.reserve(reserve_to);
        }
        if (index2entity_.capacity() < reserve_to) {
            index2entity_.reserve(reserve_to);
        }
    }

    // rollback

    ATOM_CONSTEXPR_SINCE_CXX26 void
        _destroy_prefix(size_type count, size_type index) noexcept {
        for (size_type kind = 0; kind < count; ++kind) {
            dtors_[kind](storage_[kind], index, 1);
        }
    }

    ATOM_CONSTEXPR_SINCE_CXX26 void
        _rollback_appended(size_type original_size) noexcept {
        const auto appended = size_ - original_size;
        if (appended == 0) {
            return;
        }

        for (size_type kind = 0; kind < kinds_; ++kind) {
            dtors_[kind](storage_[kind], original_size, appended);
        }

        while (size_ > original_size) {
            entity2index_.erase(index2entity_.back());
            index2entity_.pop_back();
            --size_;
        }
    }

    template <typename OnRollback>
    ATOM_CONSTEXPR_SINCE_CXX26 void _rollback_appended(
        size_type original_size, OnRollback&& on_rollback) noexcept {
        static_assert(
            std::is_nothrow_invocable_v<OnRollback&, entity_t>,
            "rollback hooks must be noexcept");

        for (size_type index = size_; index != original_size; --index) {
            on_rollback(index2entity_[index - 1]);
        }
        _rollback_appended(original_size);
    }

    template <typename OnRollback>
    constexpr void _rollback_staged_entities(
        size_type original_size, OnRollback&& on_rollback) noexcept {
        static_assert(
            std::is_nothrow_invocable_v<OnRollback&, entity_t>,
            "rollback hooks must be noexcept");

        while (index2entity_.size() > original_size) {
            on_rollback(index2entity_.back());
            entity2index_.erase(index2entity_.back());
            index2entity_.pop_back();
        }
    }

    // transfer

    template <component... Components, typename Tup>
    ATOM_CONSTEXPR_SINCE_CXX26 void _construct_added_from_tuple(
        size_type kind, size_type index, _hash_type hash, Tup&& tuple) {
        void* const column = storage_[kind];
        const bool matched =
            ((hash == hash_of<Components>()
                  ? (_construct_from_tuple<Components>(
                         column, index, std::forward<Tup>(tuple)),
                     true)
                  : false) ||
             ...);
        ATOM_ASSERT(matched && "missing constructor for added component");
        (void)matched;
    }

    template <typename Ty, typename Tup>
    ATOM_CONSTEXPR_SINCE_CXX26 static void
        _construct_from_tuple(void* column, size_type index, Tup&& tuple) {
        if constexpr (!std::is_empty_v<Ty>) {
            std::construct_at(
                _archetype::_column<Ty>(column) + index,
                rmcvref_first<Ty>(std::forward<Tup>(tuple)));
        }
    }

    template <typename MissingCtor>
    ATOM_CONSTEXPR_SINCE_CXX26 void _transfer(
        entity_t entity, archetype& target, MissingCtor&& missing_ctor) {
        const auto source_index = entity2index_.at(entity);
        const auto target_index = target.size_;
        target._ensure_capacity_for_one();

        size_type source_kind = 0;
        size_type target_kind = 0;
        size_type constructed = 0;

        auto guard = make_exception_guard(
            [&target, target_index, &constructed]() noexcept {
                target._destroy_prefix(constructed, target_index);
            });

        while (target_kind < target.kinds_) {
            if (source_kind < kinds_ &&
                hash_list_[source_kind] < target.hash_list_[target_kind]) {
                ++source_kind;
                continue;
            }

            if (source_kind < kinds_ &&
                hash_list_[source_kind] == target.hash_list_[target_kind]) {
                target.mov_ctors_[target_kind](
                    storage_[source_kind], source_index,
                    target.storage_[target_kind], target_index, 1);
                ++source_kind;
            } else {
                missing_ctor(
                    target_kind, target_index, target.hash_list_[target_kind]);
            }

            ++target_kind;
            ++constructed;
        }

        target.entity2index_.try_emplace(entity, target_index);
        target.index2entity_.push_back(entity);
        ++target.size_;

        guard.dismiss();
        _erase_known_index(entity, source_index);
    }

    // emplace one

    /**
     * @brief Records `entity` as row `size_` before its components exist.
     *
     * Recording may allocate, so it goes first: a failure leaves no
     * component to destroy.
     */
    ATOM_CONSTEXPR_SINCE_CXX26 void _stage_entity(entity_t entity) {
        entity2index_.try_emplace(entity, size_);
        auto guard = make_exception_guard([this, entity]() noexcept {
            entity2index_.erase(entity);
        });
        index2entity_.push_back(entity);
        guard.dismiss();
    }

    /// @brief Undoes `_stage_entity` when the components fail to construct.
    constexpr void _unstage_entity(entity_t entity) noexcept {
        entity2index_.erase(entity);
        index2entity_.pop_back();
    }

    // emplace(entity);

    ATOM_CONSTEXPR_SINCE_CXX26 auto _emplace(entity_t entity) {
        if (size_ == capacity_) [[unlikely]] {
            _relocate(capacity_ << 1);
        }
        _stage_entity(entity);
        auto guard = make_exception_guard([this, entity]() noexcept {
            _unstage_entity(entity);
        });
        _emplace_normally();
        guard.dismiss();
        ++size_;
    }

    ATOM_CONSTEXPR_SINCE_CXX26 auto _emplace_normally() {
        size_type idx = 0;
        // On throw, kinds [0, idx) were constructed at row `size_`.
        auto guard = make_exception_guard([this, &idx]() noexcept {
            for (size_type i = idx; i-- > 0;) {
                dtors_[i](storage_[i], size_, 1);
            }
        });

        for (; idx < kinds_; ++idx) {
            ctors_[idx](storage_[idx], size_, 1);
        }

        guard.dismiss();
    }

    // emplace<...>();

    template <component... Components>
    ATOM_CONSTEXPR_SINCE_CXX26 auto
        _emplace(entity_t entity, [[maybe_unused]] type_list<Components...>) {
        if (size_ == capacity_) [[unlikely]] {
            _relocate_as<Components...>(capacity_ << 1);
        }
        _stage_entity(entity);
        auto guard = make_exception_guard([this, entity]() noexcept {
            _unstage_entity(entity);
        });
        _archetype::_emplace_one_normally<Components...>(
            std::span<void* const, sizeof...(Components)>{ storage_, kinds_ },
            size_);
        guard.dismiss();
        ++size_;
    }

    // emplace<...>(range);

    template <
        size_t Index, typename TypeList,
        typename Ty = type_list_element_t<Index, TypeList>>
    ATOM_CONSTEXPR_SINCE_CXX26 void
        _emplace_n_noexcept(size_type count) noexcept {
        if constexpr (!std::is_empty_v<Ty>) {
            std::uninitialized_value_construct_n(
                _archetype::_column<Ty>(storage_[Index]) + size_, count);
        }
    }

    template <
        compatible_range<entity_t> Rng, component... Components,
        typename BindEntity, typename UnbindEntity>
    requires(std::is_nothrow_default_constructible_v<Components> && ...)
    ATOM_CONSTEXPR_SINCE_CXX26 auto _emplace_with_entity_binding(
        Rng&& range, size_type append,
        [[maybe_unused]] const type_list<Components...> tl,
        BindEntity&& bind_entity, UnbindEntity&& unbind_entity) {

        const auto currsize = size_;
        auto index          = currsize;
        index2entity_.reserve(currsize + append);
        entity2index_.reserve(currsize + append);
        auto guard =
            make_exception_guard([this, currsize, &unbind_entity]() noexcept {
                _rollback_staged_entities(currsize, unbind_entity);
            });
        for (entity_t entity : range) {
            entity2index_.try_emplace(entity, index++);
            index2entity_.push_back(entity);
            bind_entity(entity);
        }

        // noexcept part
        [this, append]<size_t... Is>(std::index_sequence<Is...>) {
            (_emplace_n_noexcept<Is, type_list<Components...>>(append), ...);
        }(std::index_sequence_for<Components...>());
        guard.dismiss();
    }

    template <
        compatible_range<entity_t> Rng, component... Components,
        typename BindEntity, typename UnbindEntity>
    requires(!(std::is_nothrow_default_constructible_v<Components> && ...))
    ATOM_CONSTEXPR_SINCE_CXX26 auto _emplace_with_entity_binding(
        Rng&& range, size_type append,
        [[maybe_unused]] const type_list<Components...> tl,
        BindEntity&& bind_entity, UnbindEntity&& unbind_entity) {

        const auto currsize = size_;
        auto index          = currsize;
        index2entity_.reserve(currsize + append);
        entity2index_.reserve(currsize + append);
        auto guard =
            make_exception_guard([this, currsize, &unbind_entity]() noexcept {
                _rollback_staged_entities(currsize, unbind_entity);
            });
        for (entity_t entity : range) {
            entity2index_.try_emplace(entity, index++);
            index2entity_.push_back(entity);
            bind_entity(entity);
        }

        using clist        = type_list<Components...>;
        using isequence    = std::index_sequence_for<Components...>;
        size_type finished = 0;
        auto clean         = [this, &finished, currsize, append]() noexcept {
            [this, &finished, currsize,
             append]<size_t... Is>(std::index_sequence<Is...>) {
                (_clean_for_emplace_n<Is, clist>(finished, currsize, append),
                 ...);
            }(isequence());
        };
        auto emplace_guard = make_exception_guard(clean);
        [this, &finished, append]<size_t... Is>(std::index_sequence<Is...>) {
            (_emplace_n<Is, clist>(finished, append), ...);
        }(isequence());
        emplace_guard.dismiss();

        guard.dismiss();
    }

    template <
        compatible_range<entity_t> Rng, component... Components,
        typename BindEntity, typename UnbindEntity>
    ATOM_CONSTEXPR_SINCE_CXX26 auto _emplace_with_entity_binding(
        Rng&& range, [[maybe_unused]] const type_list<Components...> tl,
        BindEntity&& bind_entity, UnbindEntity&& unbind_entity) {
        if constexpr (std::ranges::sized_range<Rng>) {
            auto append     = std::ranges::size(range);
            auto final_size = size_ + append;
            entity2index_.reserve(final_size);
            index2entity_.reserve(final_size);
            if (final_size > capacity_) {
                _relocate_as<Components...>(
                    (std::max)(final_size, capacity_ << 1));
            }
            _emplace_with_entity_binding(
                std::forward<Rng>(range), append, tl,
                std::forward<BindEntity>(bind_entity),
                std::forward<UnbindEntity>(unbind_entity));
            size_ += append;
        } else {
            const auto original_size = size_;
            auto guard               = make_exception_guard(
                [this, original_size, &unbind_entity]() noexcept {
                    _rollback_appended(original_size, unbind_entity);
                });
            std::ranges::for_each(range, [this, &bind_entity](entity_t entity) {
                _ensure_capacity_for_one();
                _emplace(entity, type_list<Components...>{});
                bind_entity(entity);
            });
            guard.dismiss();
        }
    }

    template <
        component... Components, compatible_range<entity_t> Rng,
        typename BindEntity, typename UnbindEntity>
    ATOM_CONSTEXPR_SINCE_CXX26 auto _emplace_with_entity_binding(
        Rng&& range, BindEntity&& bind_entity, UnbindEntity&& unbind_entity) {
        using type_list = type_list<Components...>;
        using hash_list = hash_list_t<type_list>;
        _emplace_with_entity_binding(
            std::forward<Rng>(range), hash_list{},
            std::forward<BindEntity>(bind_entity),
            std::forward<UnbindEntity>(unbind_entity));
    }

    template <compatible_range<entity_t> Rng, component... Components>
    ATOM_CONSTEXPR_SINCE_CXX26 auto _emplace(
        Rng&& range, [[maybe_unused]] const type_list<Components...> tl) {
        auto ignore_entity = [](entity_t) noexcept {
        };
        _emplace_with_entity_binding(
            std::forward<Rng>(range), tl, ignore_entity, ignore_entity);
    }

    template <
        size_t Index, typename TypeList,
        typename Ty = type_list_element_t<Index, TypeList>>
    ATOM_CONSTEXPR_SINCE_CXX26 void _clean_for_emplace_n(
        size_type finished, size_type original_size, size_type count) noexcept {
        if constexpr (!std::is_empty_v<Ty>) {
            if (Index < finished) {
                std::destroy_n(
                    _archetype::_column<Ty>(storage_[Index]) + original_size,
                    count);
            }
        }
    }

    template <
        size_t Index, typename TypeList,
        typename Ty = type_list_element_t<Index, TypeList>>
    ATOM_CONSTEXPR_SINCE_CXX26 void
        _emplace_n(size_type& finished, size_type size_appending) {
        if constexpr (!std::is_empty_v<Ty>) {
            std::uninitialized_value_construct_n(
                _archetype::_column<Ty>(storage_[Index]) + size_,
                size_appending);
        }

        ++finished;
    }

    // emplace(...);

    template <component... Components>
    ATOM_CONSTEXPR_SINCE_CXX26 void
        _emplace(entity_t entity, Components&&... comps) {
        if (size_ == capacity_) [[unlikely]] {
            _relocate_as<Components...>(capacity_ << 1);
        }
        _stage_entity(entity);
        auto guard = make_exception_guard([this, entity]() noexcept {
            _unstage_entity(entity);
        });
        _archetype::_emplace_one_normally(
            std::span<void* const, sizeof...(Components)>{ storage_, kinds_ },
            size_, std::forward<Components>(comps)...);
        guard.dismiss();
        ++size_;
    }

    template <component... Components, typename Tup>
    ATOM_CONSTEXPR_SINCE_CXX26 void
        _emplace(entity_t entity, type_list<Components...>, Tup&& vals) {
        if (size_ == capacity_) [[unlikely]] {
            _relocate_as<Components...>(capacity_ << 1);
        }
        _stage_entity(entity);
        auto guard = make_exception_guard([this, entity]() noexcept {
            _unstage_entity(entity);
        });
        _archetype::_emplace_one_normally<Components...>(
            std::span<void* const, sizeof...(Components)>{ storage_, kinds_ },
            size_, std::forward<Tup>(vals));
        guard.dismiss();
        ++size_;
    }

    // emplace(range, ...)

    template <
        compatible_range<entity_t> Rng, typename BindEntity,
        typename UnbindEntity, component... Components>
    ATOM_CONSTEXPR_SINCE_CXX26 void _emplace_with_entity_binding(
        Rng&& range, BindEntity&& bind_entity, UnbindEntity&& unbind_entity,
        Components&&... components) {
        using sorted =
            hash_list_t<type_list<std::remove_cvref_t<Components>...>>;

        if constexpr (std::ranges::sized_range<Rng>) {
            const auto append     = std::ranges::size(range);
            const auto final_size = size_ + append;
            entity2index_.reserve(final_size);
            index2entity_.reserve(final_size);
            if (final_size > capacity_) {
                _relocate_as<std::remove_cvref_t<Components>...>(
                    (std::max)(final_size, capacity_ << 1));
            }
        }

        auto tuple =
            std::forward_as_tuple(std::forward<Components>(components)...);
        const auto original_size = size_;
        auto guard               = make_exception_guard(
            [this, original_size, &unbind_entity]() noexcept {
                _rollback_appended(original_size, unbind_entity);
            });

        for (entity_t entity : range) {
            if constexpr (!std::ranges::sized_range<Rng>) {
                _ensure_capacity_for_one();
            }
            _emplace(entity, sorted{}, tuple);
            bind_entity(entity);
        }

        guard.dismiss();
    }

    template <compatible_range<entity_t> Rng, component... Components>
    ATOM_CONSTEXPR_SINCE_CXX26 void
        _emplace(Rng&& range, Components&&... components) {
        auto ignore_entity = [](entity_t) noexcept {
        };
        _emplace_with_entity_binding(
            std::forward<Rng>(range), ignore_entity, ignore_entity,
            std::forward<Components>(components)...);
    }

    // views

    /// @brief A missing kind would index past the last column.
    static constexpr void _assert_stored(
        [[maybe_unused]] const _hash_type* found,
        [[maybe_unused]] const _hash_type* end,
        [[maybe_unused]] _hash_type hash) noexcept {
        ATOM_ASSERT(
            found != end && *found == hash &&
            "the archetype does not store the component");
    }

    /// @brief Index of each of the hash-sorted `Components` among the
    /// kinds.
    template <component... Components>
    ATOM_ATTRS(ATOM_NODISCARD)
    constexpr auto _kind_indices(type_list<Components...>) const noexcept
        -> std::array<size_type, sizeof...(Components)> {
        std::array<size_type, sizeof...(Components)> result{};
        const _hash_type* hint      = hash_list_;
        const _hash_type* const end = hash_list_ + kinds_;
        size_type i                 = 0;
        ((hint = branchless_lower_bound(hint, end, hash_of<Components>()),
          _assert_stored(hint, end, hash_of<Components>()),
          result[i++] = static_cast<size_type>(hint - hash_list_)),
         ...);
        return result;
    }

    template <component... Components>
    ATOM_ATTRS(ATOM_NODISCARD)
    ATOM_CONSTEXPR_SINCE_CXX26 auto _get()
        -> std::tuple<std::remove_cvref_t<Components>*...> {
        using tlist     = type_list<std::remove_cvref_t<Components>...>;
        using hash_list = hash_list_t<tlist>;
        const auto kind = _kind_indices(hash_list{});
        using isequence = hash_sequence_t<tlist>;
        using vlist     = value_list_from_t<isequence>;
        return [this, &kind]<size_t... Is>(std::index_sequence<Is...>) {
            return std::make_tuple(
                _archetype::_column<std::remove_cvref_t<Components>>(
                    storage_[kind[value_list_element_v<Is, vlist>]])...);
        }(std::index_sequence_for<Components...>());
    }

    template <component... Components>
    ATOM_ATTRS(ATOM_NODISCARD)
    constexpr auto _get_buffers() noexcept {
        using tlist     = type_list<std::remove_cvref_t<Components>...>;
        using hash_list = hash_list_t<tlist>;
        const auto kind = _kind_indices(hash_list{});
        using isequence = hash_sequence_t<tlist>;
        using vlist     = value_list_from_t<isequence>;
        return [this, &kind]<size_t... Is>(std::index_sequence<Is...>) {
            return std::make_tuple(
                static_cast<void* const*>(
                    storage_ + kind[value_list_element_v<Is, vlist>])...);
        }(std::index_sequence_for<Components...>());
    }

    ATOM_ATTRS(ATOM_NODISCARD)
    auto _entities() const noexcept -> const entity_t* const& {
        return index2entity_.data();
    }

    /// @brief Typed relocation routine, set when the kinds are known at
    /// compile time.
    void (*relocate_fn_)(archetype*, size_type) = nullptr;
    ATOM_NO_UNIQUE_ADDR allocator_type alloc_;
    // Per-kind metadata, `kinds_` entries each, sorted by hash.
    _hash_type* hash_list_       = nullptr;
    type_traits* comp_traits_    = nullptr;
    _alloc_fn* allocs_           = nullptr;
    _dealloc_fn* deallocs_       = nullptr;
    _ctor_fn* ctors_             = nullptr;
    _mov_ctor_fn* mov_ctors_     = nullptr;
    _mov_assign_fn* mov_assigns_ = nullptr;
    _dtor_fn* dtors_             = nullptr;
    /// @brief One column per kind; null for empty components.
    void** storage_     = nullptr;
    size_type kinds_    = 0;
    size_type size_     = 0;
    size_type capacity_ = 0;
    std::uint64_t hash_ = 0;
    shift_map<entity_t, index_t, 256UL, half_bits<entity_t>, Alloc>
        entity2index_;
    anchor<entity_t, _allocator_t<entity_t>> index2entity_;
};

template <unfancy_byte_allocator Alloc>
template <component... Components, typename Al>
requires(sizeof...(Components) != 0)
ATOM_CONSTEXPR_SINCE_CXX26 archetype<Alloc>::archetype(
    immediately_t, type_list<Components...>, const Al& alloc)
    : relocate_fn_([](archetype* self, size_type capacity) {
          self->template _relocate_as<Components...>(capacity);
      }),
      alloc_(alloc), hash_(make_array_hash<type_list<Components...>>()),
      entity2index_(alloc), index2entity_(alloc) {
    using table = _kind_table<Components...>;

    _allocate_kinds(table::count);
    auto guard = make_exception_guard([this]() noexcept {
        _release(initial_capacity);
    });
    for (size_type i = 0; i < table::count; ++i) {
        _set_kind<table>(i, i);
    }
    _allocate_columns(initial_capacity);
    capacity_ = initial_capacity;
    guard.dismiss();
}

template <unfancy_byte_allocator Alloc>
template <component... Components>
ATOM_CONSTEXPR_SINCE_CXX26 archetype<Alloc>::archetype(
    const archetype& that, add_components_t<Components...>)
    : alloc_(that.alloc_), entity2index_(that.alloc_),
      index2entity_(that.alloc_) {
    using table = type_list_rebind_t<
        _kind_table,
        hash_list_t<type_list<std::remove_cvref_t<Components>...>>>;
    constexpr auto& added = table::hashes;
    const auto* const own = that.hash_list_;
    const size_type kinds = that.kinds_;

    // Merge the two sorted lists: first count, then fill.
    auto merge = [&](auto&& keep_own, auto&& keep_added) {
        size_type i = 0;
        size_type j = 0;
        size_type k = 0;
        while (i < kinds || j < added.size()) {
            if (j == added.size() || (i < kinds && own[i] < added[j])) {
                keep_own(k++, i++);
            } else if (i == kinds || added[j] < own[i]) {
                keep_added(k++, j++);
            } else {
                keep_own(k++, i++);
                ++j;
            }
        }
        return k;
    };
    auto ignore = [](size_type, size_type) noexcept {
    };

    _allocate_kinds(merge(ignore, ignore));
    auto guard = make_exception_guard([this]() noexcept {
        _release(initial_capacity);
    });
    merge(
        [this, &that](size_type k, size_type i) noexcept {
            _copy_kind(k, that, i);
        },
        [this](size_type k, size_type j) noexcept {
            _set_kind<table>(k, j);
        });
    _allocate_columns(initial_capacity);
    capacity_ = initial_capacity;          // NOLINT
    hash_     = hash_combine(hash_list()); // NOLINT
    guard.dismiss();
}

template <unfancy_byte_allocator Alloc>
template <component... Components, typename Al>
requires(sizeof...(Components) != 0)
ATOM_CONSTEXPR_SINCE_CXX26
    archetype<Alloc>::archetype(type_spreader<Components...>, const Al& alloc)
    : archetype(immediately, hash_list_t<type_list<Components...>>{}, alloc) {}

template <unfancy_byte_allocator Alloc>
template <component... Components>
ATOM_CONSTEXPR_SINCE_CXX26 archetype<Alloc>::archetype(
    const archetype& that, remove_components_t<Components...>)
    : alloc_(that.alloc_), entity2index_(that.alloc_),
      index2entity_(that.alloc_) {
    constexpr auto removed =
        make_hash_array<type_list<std::remove_cvref_t<Components>...>>();
    const auto* const own = that.hash_list_;
    const size_type kinds = that.kinds_;

    auto filter = [&](auto&& keep) {
        size_type j = 0;
        size_type k = 0;
        for (size_type i = 0; i < kinds; ++i) {
            while (j < removed.size() && removed[j] < own[i]) {
                ++j;
            }
            if (j == removed.size() || removed[j] != own[i]) {
                keep(k++, i);
            }
        }
        return k;
    };

    _allocate_kinds(filter([](size_type, size_type) noexcept {
    }));
    auto guard = make_exception_guard([this]() noexcept {
        _release(initial_capacity);
    });
    filter([this, &that](size_type k, size_type i) noexcept {
        _copy_kind(k, that, i);
    });
    _allocate_columns(initial_capacity);
    capacity_ = initial_capacity;          // NOLINT
    hash_     = hash_combine(hash_list()); // NOLINT
    guard.dismiss();
}

template <unfancy_byte_allocator Alloc>
ATOM_CONSTEXPR_SINCE_CXX20
    archetype<Alloc>::archetype(archetype&& that) noexcept
    : relocate_fn_(that.relocate_fn_), alloc_(that.alloc_),
      hash_list_(std::exchange(that.hash_list_, nullptr)),
      comp_traits_(std::exchange(that.comp_traits_, nullptr)),
      allocs_(std::exchange(that.allocs_, nullptr)),
      deallocs_(std::exchange(that.deallocs_, nullptr)),
      ctors_(std::exchange(that.ctors_, nullptr)),
      mov_ctors_(std::exchange(that.mov_ctors_, nullptr)),
      mov_assigns_(std::exchange(that.mov_assigns_, nullptr)),
      dtors_(std::exchange(that.dtors_, nullptr)),
      storage_(std::exchange(that.storage_, nullptr)),
      kinds_(std::exchange(that.kinds_, 0)),
      size_(std::exchange(that.size_, 0)),
      capacity_(std::exchange(that.capacity_, 0)),
      hash_(std::exchange(that.hash_, 0)),
      entity2index_(std::move(that.entity2index_)),
      index2entity_(std::move(that.index2entity_)) {}

template <unfancy_byte_allocator Alloc>
ATOM_CONSTEXPR_SINCE_CXX26 archetype<Alloc>::~archetype() noexcept {
    for (size_type i = 0; i < kinds_; ++i) {
        dtors_[i](storage_[i], 0, size_);
    }
    _release(capacity_);
}

template <unfancy_byte_allocator Alloc>
template <typename... Args>
ATOM_ATTRS(ATOM_NODISCARD)
constexpr auto archetype<Alloc>::has() const noexcept {
    using sorted_list = hash_list_t<type_list<Args...>>;
    return _has_impl<sorted_list>();
}

template <unfancy_byte_allocator Alloc>
ATOM_ATTRS(ATOM_NODISCARD)
ATOM_CONSTEXPR_SINCE_CXX26 auto archetype<Alloc>::emplace(entity_t entity) {
    _emplace(entity);
}

template <unfancy_byte_allocator Alloc>
template <component... Components>
ATOM_ATTRS(ATOM_NODISCARD)
ATOM_CONSTEXPR_SINCE_CXX26 auto archetype<Alloc>::emplace(entity_t entity) {
    using type_list                       = type_list<Components...>;
    using hash_list                       = hash_list_t<type_list>;
    constexpr std::uint64_t combined_hash = make_array_hash<type_list>();
    ATOM_ASSUME(combined_hash == hash_);

    _emplace(entity, hash_list{});
}

template <unfancy_byte_allocator Alloc>
template <component... Components>
ATOM_ATTRS(ATOM_NODISCARD)
ATOM_CONSTEXPR_SINCE_CXX26 auto archetype<Alloc>::emplace(auto&& range)
requires compatible_range<decltype(range), entity_t>
{
    using type_list                       = type_list<Components...>;
    using hash_list                       = hash_list_t<type_list>;
    constexpr std::uint64_t combined_hash = make_array_hash<type_list>();
    ATOM_ASSERT(combined_hash == hash_);
    ATOM_ASSUME(combined_hash == hash_);

    _emplace(std::forward<decltype(range)>(range), hash_list{});
}

template <unfancy_byte_allocator Alloc>
template <component... Components>
ATOM_ATTRS(ATOM_NODISCARD)
ATOM_CONSTEXPR_SINCE_CXX26 auto archetype<Alloc>::emplace(
    entity_t entity, Components&&... components) {
    constexpr std::uint64_t combined_hash =
        make_array_hash<type_list<std::remove_cvref_t<Components>...>>();
    ATOM_ASSERT(combined_hash == hash_);
    ATOM_ASSUME(combined_hash == hash_);

    _emplace(entity, std::forward<Components>(components)...);
}

template <unfancy_byte_allocator Alloc>
template <component... Components>
ATOM_ATTRS(ATOM_NODISCARD)
ATOM_CONSTEXPR_SINCE_CXX26
    auto archetype<Alloc>::emplace(auto&& range, Components&&... components)
requires compatible_range<decltype(range), entity_t>
{
    constexpr std::uint64_t combined_hash =
        make_array_hash<type_list<std::remove_cvref_t<Components>...>>();
    ATOM_ASSERT(combined_hash == hash_);
    ATOM_ASSUME(combined_hash == hash_);

    _emplace(
        std::forward<decltype(range)>(range),
        std::forward<Components>(components)...);
}

template <unfancy_byte_allocator Alloc>
ATOM_CONSTEXPR_SINCE_CXX26 void archetype<Alloc>::erase(entity_t entity) {
    _erase_known_index(entity, entity2index_.at(entity));
}

template <unfancy_byte_allocator Alloc>
template <component... Components>
requires(!std::is_empty_v<std::remove_cvref_t<Components>> && ...)
ATOM_ATTRS(ATOM_NODISCARD)
ATOM_CONSTEXPR_SINCE_CXX26 auto archetype<Alloc>::at(entity_t entity)
    -> std::tuple<Components...> {
    const auto index = entity2index_.at(entity);
    auto storage     = _get<Components...>();
    return std::apply(
        [index](auto*... ptrs) -> std::tuple<Components...> {
            return std::tuple<Components...>(ptrs[index]...);
        },
        storage);
}

template <unfancy_byte_allocator Alloc>
ATOM_ATTRS(ATOM_NODISCARD)
constexpr long_hash_t archetype<Alloc>::hash() const noexcept {
    return hash_;
}

template <unfancy_byte_allocator Alloc>
ATOM_ATTRS(ATOM_NODISCARD)
constexpr auto archetype<Alloc>::hash_list() const noexcept
    -> std::span<const _hash_type> {
    return { hash_list_, kinds_ };
}

template <unfancy_byte_allocator Alloc>
ATOM_ATTRS(ATOM_NODISCARD)
constexpr void* const* archetype<Alloc>::data() const noexcept {
    return storage_;
}

template <unfancy_byte_allocator Alloc>
ATOM_ATTRS(ATOM_NODISCARD)
constexpr auto archetype<Alloc>::size() const noexcept -> const size_type& {
    return size_;
}

template <unfancy_byte_allocator Alloc>
ATOM_ATTRS(ATOM_NODISCARD)
constexpr bool archetype<Alloc>::empty() const noexcept {
    return size_ == 0UL;
}

template <unfancy_byte_allocator Alloc>
ATOM_ATTRS(ATOM_NODISCARD)
constexpr auto archetype<Alloc>::capacity() const noexcept
    -> archetype<Alloc>::size_type {
    return capacity_;
}

template <unfancy_byte_allocator Alloc>
template <component... Components>
ATOM_ATTRS(ATOM_NODISCARD)
constexpr auto archetype<Alloc>::view_buffers() noexcept {
    return _get_buffers<Components...>();
}

template <unfancy_byte_allocator Alloc>
ATOM_ATTRS(ATOM_NODISCARD)
constexpr auto archetype<Alloc>::entities() noexcept -> std::span<entity_t> {
    return index2entity_;
}

template <unfancy_byte_allocator Alloc>
ATOM_CONSTEXPR_SINCE_CXX26 void archetype<Alloc>::reserve(size_type n) {
    entity2index_.reserve(n);
    index2entity_.reserve(n);
    if (capacity_ >= n) {
        return;
    }

    _relocate(n);
}

template <unfancy_byte_allocator Alloc>
ATOM_CONSTEXPR_SINCE_CXX26 void archetype<Alloc>::clear() {
    for (size_type i = 0; i < kinds_; ++i) {
        dtors_[i](storage_[i], 0, size_);
    }
    index2entity_.clear();
    entity2index_.clear();
    size_ = 0;
}

template <unfancy_byte_allocator Alloc>
ATOM_ATTRS(ATOM_NODISCARD)
constexpr auto archetype<Alloc>::get_allocator() const noexcept
    -> allocator_type {
    return alloc_;
}

template <unfancy_byte_allocator Alloc>
ATOM_CONSTEXPR_SINCE_CXX26 void
    archetype<Alloc>::transfer(entity_t entity, archetype& target) {
    _transfer(entity, target, [](size_type, size_type, _hash_type) {
        ATOM_ASSERT(
            false && "target archetype unexpectedly requires new components");
    });
}

template <unfancy_byte_allocator Alloc>
template <component... Components>
ATOM_CONSTEXPR_SINCE_CXX26 void archetype<Alloc>::transfer(
    entity_t entity, archetype& target,
    [[maybe_unused]] add_components_t<Components...>) {
    _transfer(
        entity, target, [&target](size_type kind, size_type index, _hash_type) {
            target.ctors_[kind](target.storage_[kind], index, 1);
        });
}

template <unfancy_byte_allocator Alloc>
template <component... Added, component... Components>
ATOM_CONSTEXPR_SINCE_CXX26 void archetype<Alloc>::transfer(
    entity_t entity, archetype& target,
    [[maybe_unused]] add_components_t<Added...>, Components&&... components) {
    auto tuple = std::forward_as_tuple(std::forward<Components>(components)...);
    _transfer(
        entity, target,
        [&target, &tuple](size_type kind, size_type index, _hash_type hash) {
            target.template _construct_added_from_tuple<
                std::remove_cvref_t<Added>...>(kind, index, hash, tuple);
        });
}

// NOLINTEND(modernize-avoid-c-arrays)
// NOLINTEND(cppcoreguidelines-avoid-c-arrays)
// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

namespace pmr {

using archetype = archetype<std::pmr::polymorphic_allocator<>>;

} // namespace pmr

} // namespace neutron

namespace std {

template <neutron::std_simple_allocator Alloc>
struct uses_allocator<neutron::archetype<Alloc>, Alloc> : std::true_type {};

} // namespace std
