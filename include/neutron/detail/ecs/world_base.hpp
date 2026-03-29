// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>
#include <neutron/concepts.hpp>
#include <neutron/reflection.hpp>
#include "neutron/detail/ecs/archetype.hpp"
#include "neutron/detail/ecs/component.hpp"
#include "neutron/detail/ranges/concepts.hpp"
#include "neutron/flat_hash_map.hpp"
#include "neutron/memory.hpp"
#include "neutron/metafn.hpp"
#include "neutron/utility.hpp"

namespace neutron {

struct world_accessor;

namespace _world_base {

enum class _transition_op : uint8_t {
    add,
    remove,
};

struct _hash_transition {
    uint64_t from;
    uint64_t delta;
    _transition_op op;

    constexpr bool operator==(const _hash_transition& that) const noexcept {
        return from == that.from && delta == that.delta && op == that.op;
    }
};

ATOM_FORCE_INLINE static constexpr generation_t
    _get_gen(entity_t entity) noexcept {
    return static_cast<generation_t>(entity >> 32U);
}

ATOM_FORCE_INLINE static constexpr index_t
    _get_index(entity_t entity) noexcept {
    return static_cast<index_t>(entity);
}

ATOM_FORCE_INLINE static constexpr void
    _reset_index(entity_t& entity) noexcept {
    entity = ((entity >> 32) << 32);
}

ATOM_NODISCARD ATOM_FORCE_INLINE constexpr static entity_t
    _make_entity(generation_t gen, index_t index) noexcept {
    return (static_cast<entity_t>(gen) << 32UL) | index;
}

template <std_simple_allocator Alloc>
class _spawn_n_result {
    template <std_simple_allocator>
    friend class _entity_slots_base;

    using allocator_type   = neutron::rebind_alloc_t<Alloc, entity_t>;
    using allocator_traits = std::allocator_traits<allocator_type>;

public:
    using value_type      = entity_t;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    class _iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept  = std::forward_iterator_tag;
        using value_type        = entity_t;
        using difference_type   = ptrdiff_t;
        using reference         = entity_t;

        constexpr _iterator() noexcept = default;

        constexpr _iterator(
            const _spawn_n_result* range, size_type index) noexcept
            : range_(range), index_(index) {}

        ATOM_NODISCARD constexpr entity_t operator*() const noexcept {
            return (*range_)[index_];
        }

        constexpr _iterator& operator++() noexcept {
            ++index_;
            return *this;
        }

        constexpr _iterator operator++(int) noexcept {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        constexpr bool operator==(const _iterator& that) const noexcept {
            return range_ == that.range_ && index_ == that.index_;
        }

    private:
        const _spawn_n_result* range_{};
        size_type index_{};
    };

    constexpr _spawn_n_result() noexcept = default;

    constexpr _spawn_n_result(const _spawn_n_result&)            = delete;
    constexpr _spawn_n_result& operator=(const _spawn_n_result&) = delete;

    constexpr _spawn_n_result(_spawn_n_result&& that) noexcept
        : recycled_(std::exchange(that.recycled_, nullptr)),
          recycled_count_(std::exchange(that.recycled_count_, 0)),
          fresh_begin_(std::exchange(that.fresh_begin_, 0)),
          fresh_count_(std::exchange(that.fresh_count_, 0)),
          allocator_(std::move(that.allocator_)) {}

    constexpr _spawn_n_result& operator=(_spawn_n_result&& that) noexcept {
        if (this != &that) {
            _destroy();
            recycled_       = std::exchange(that.recycled_, nullptr);
            recycled_count_ = std::exchange(that.recycled_count_, 0);
            fresh_begin_    = std::exchange(that.fresh_begin_, 0);
            fresh_count_    = std::exchange(that.fresh_count_, 0);
            allocator_      = std::move(that.allocator_);
        }
        return *this;
    }

    constexpr ~_spawn_n_result() noexcept { _destroy(); }

    ATOM_NODISCARD constexpr auto begin() const noexcept {
        return _iterator(this, 0);
    }

    ATOM_NODISCARD constexpr auto end() const noexcept {
        return _iterator(this, size());
    }

    ATOM_NODISCARD constexpr size_type size() const noexcept {
        return recycled_count_ + fresh_count_;
    }

    ATOM_NODISCARD constexpr bool empty() const noexcept { return size() == 0; }

    ATOM_NODISCARD constexpr entity_t
        operator[](size_type index) const noexcept {
        if (index < recycled_count_) {
            return recycled_[index]; // NOLINT, perf
        }

        const auto offset = static_cast<index_t>(index - recycled_count_);
        return _make_entity(0, fresh_begin_ + offset);
    }

private:
    constexpr _spawn_n_result(
        size_type recycled_count, index_t fresh_begin, size_type fresh_count,
        const allocator_type& alloc)
        : recycled_count_(recycled_count), fresh_begin_(fresh_begin),
          fresh_count_(fresh_count), allocator_(alloc) {
        if (recycled_count_ != 0) {
            recycled_ = allocator_traits::allocate(allocator_, recycled_count_);
        }
    }

    constexpr void _destroy() noexcept {
        if (recycled_ != nullptr) {
            allocator_traits::deallocate(
                allocator_, recycled_, recycled_count_);
            recycled_ = nullptr;
        }
        recycled_count_ = 0;
        fresh_begin_    = 0;
        fresh_count_    = 0;
    }

    entity_t* recycled_{};
    size_type recycled_count_{};
    index_t fresh_begin_{};
    size_type fresh_count_{};
    [[no_unique_address]] allocator_type allocator_{};
};

template <std_simple_allocator Alloc>
class _entity_slots_base {
    friend struct ::neutron::world_accessor;

public:
    using size_type = size_t;
    using archetype = ::neutron::archetype<Alloc>;

    struct entity_slot {
        entity_t first{};
        union {
            archetype* second;
            index_t next_free;
        };

        constexpr entity_slot() noexcept : second(nullptr) {}

        constexpr explicit entity_slot(
            entity_t entity, archetype* archetype = nullptr) noexcept
            : first(entity), second(archetype) {}
    };

protected:
    template <typename Ty>
    using _allocator_t = neutron::rebind_alloc_t<Alloc, Ty>;

    template <typename Ty>
    using _vector_t      = std::vector<Ty, _allocator_t<Ty>>;
    using _spawned_range = _spawn_n_result<Alloc>;

    template <typename Al = Alloc>
    explicit _entity_slots_base(const Al& alloc = Alloc{})
        : entities_(1, alloc) {}

    constexpr entity_t _get_new_entity() {
        if (free_head_ == 0) {
            const auto index = static_cast<index_t>(entities_.size());
            entities_.emplace_back(_make_entity(0, index));
            return entities_.back().first;
        }

        const index_t index = free_head_;
        auto& slot          = entities_[index];
        free_head_          = slot.next_free;
        slot.next_free      = 0;
        slot.second         = nullptr;
        slot.first          = _make_entity(_get_gen(slot.first) + 1, index);
        return slot.first;
    }

    constexpr auto _get_new_entities(size_type n) -> _spawned_range {
        using entity_alloc = typename _spawned_range::allocator_type;

        if (n == 0) {
            return _spawned_range{};
        }

        const auto recycled_count = _count_reusable_entities(n);
        const auto fresh_count    = n - recycled_count;
        auto range =
            _spawned_range{ recycled_count,
                            static_cast<index_t>(entities_.size()), fresh_count,
                            entity_alloc{ entities_.get_allocator() } };

        if (fresh_count != 0) {
            entities_.reserve(entities_.size() + fresh_count);
        }

        for (size_type i = 0; i < recycled_count; ++i) {
            const index_t index = free_head_;
            auto& slot          = entities_[index];
            free_head_          = slot.next_free;
            slot.next_free      = 0;
            slot.second         = nullptr;
            slot.first          = _make_entity(_get_gen(slot.first) + 1, index);
            range.recycled_[i]  = slot.first;
        }

        for (size_type i = 0; i < fresh_count; ++i) {
            const auto index = static_cast<index_t>(entities_.size());
            entities_.emplace_back(_make_entity(0, index));
        }

        return range;
    }

    constexpr auto _entity_slot(index_t index) noexcept -> entity_slot& {
        return entities_[index];
    }

    constexpr auto _entity_slot(index_t index) const noexcept
        -> const entity_slot& {
        return entities_[index];
    }

    constexpr void _release_entity(index_t index) noexcept {
        auto& slot = entities_[index];
        _reset_index(slot.first);
        if (_get_gen(slot.first) != (std::numeric_limits<uint32_t>::max)()) {
            slot.next_free = free_head_;
            free_head_     = index;
        } else {
            slot.next_free = 0;
        }
    }

    constexpr void _reserve_entities(size_type n) { entities_.reserve(n + 1); }

    constexpr bool _is_alive(entity_t entity) const noexcept {
        const auto index = _get_index(entity);
        return index != 0 && index < entities_.size() &&
               entities_[index].first == entity &&
               _get_index(entities_[index].first) != 0;
    }

    void _clear_entities() {
        free_head_ = 0;
        entities_.clear();
        entities_.emplace_back();
    }

    ATOM_NODISCARD constexpr size_type
        _count_reusable_entities(size_type max_count) const noexcept {
        size_type count = 0;
        for (index_t index = free_head_; index != 0 && count < max_count;
             index         = entities_[index].next_free) {
            ++count;
        }
        return count;
    }

    _vector_t<entity_slot> entities_;
    index_t free_head_{};
};

/**
 * @class world_base
 * @brief A container stores entities and their component data.
 *
 * The index of an entity starts from 1.
 * @tparam Alloc Allocator satisfy the contrains of allocator by the standard
 * library. Default is `std::allocator<std::byte>`. The container would rebind
 * allocator automatically.
 */
template <std_simple_allocator Alloc>
class world_base : private _entity_slots_base<Alloc> {
    template <typename, std_simple_allocator>
    friend class basic_world;

    friend struct ::neutron::world_accessor;

    template <typename Ty>
    using _allocator_t = neutron::rebind_alloc_t<Alloc, Ty>;

    template <typename Ty>
    using _vector_t = std::vector<Ty, _allocator_t<Ty>>;

    using _entity_slots = _entity_slots_base<Alloc>;
    using archetype     = ::neutron::archetype<Alloc>;

    struct _transition_target {
        uint64_t hash{};
        archetype* target{};
    };

    template <
        typename Kty, typename Ty, typename Hasher = std::hash<Kty>,
        typename Equal = std::equal_to<Kty>>
    using _flat_hash_map = neutron::flat_hash_map<
        Kty, Ty, Hasher, Equal, _allocator_t<std::pair<Kty, Ty>>>;

    template <
        typename Kty, typename Ty, typename Hasher = std::hash<Kty>,
        typename Equal = std::equal_to<Kty>>
    using _unordered_map = std::unordered_map<
        Kty, Ty, Hasher, Equal, _allocator_t<std::pair<const Kty, Ty>>>;

    using archetype_map = _unordered_map<uint64_t, archetype>;
    static constexpr uint64_t empty_archetype_hash =
        neutron::hash_combine(std::array<uint32_t, 0>{});
    using typename _entity_slots::entity_slot;
    using _entity_slots::_clear_entities;
    using _entity_slots::_entity_slot;
    using _entity_slots::_get_new_entity;
    using _entity_slots::_is_alive;
    using _entity_slots::_release_entity;
    using _entity_slots::_reserve_entities;
    using _entity_slots::entities_;

public:
    using size_type = typename _entity_slots::size_type;

    template <typename Al = Alloc>
    explicit world_base(const Al& alloc = Alloc{})
        : _entity_slots(alloc), archetypes_(alloc), transitions_(alloc) {}

    constexpr entity_t spawn();

    template <component... Components>
    constexpr entity_t spawn();

    template <component... Components>
    constexpr entity_t spawn(Components&&... components);

    constexpr auto spawn_n(size_type n);

    template <component... Components>
    constexpr auto spawn_n(size_type n);

    template <component... Components>
    constexpr auto spawn_n(size_type n, Components&&... components);

    template <component... Components>
    constexpr void add_components(entity_t entity);

    template <component... Components>
    constexpr void add_components(entity_t entity, Components&&... components);

    template <component... Components>
    constexpr void remove_components(entity_t entity);

    constexpr void kill(entity_t entity);

    template <compatible_range<entity_t> Rng>
    constexpr void kill(Rng&& range);

    constexpr void reserve(size_type n);

    template <component... Components>
    constexpr void reserve(size_type n);

    constexpr bool is_alive(entity_t entity) noexcept;

    void clear();

    constexpr auto get_allocator() const noexcept {
        return archetypes_.get_allocator();
    }

private:
    template <component... Components>
    constexpr void _emplace_new_entity(entity_t entity);
    template <component... Components>
    constexpr void _emplace_new_entity(entity_t entity, Components&&...);
    template <component... Components, compatible_range<entity_t> Rng>
    constexpr void _emplace_new_entities(Rng& range);
    template <component... Components, compatible_range<entity_t> Rng>
    constexpr void _emplace_new_entities(Rng& range, Components&&...);
    template <component... Components>
    auto _dst_transition_add(archetype* archetype, uint64_t delta)
        -> _transition_target&;
    template <component... Components>
    auto _dst_transition_remove(archetype* archetype, uint64_t delta)
        -> _transition_target&;

    /// @brief A container stores archetypes with combined hash.
    archetype_map archetypes_;

    /// @brief Cache for O(1) entity movement.
    _flat_hash_map<_hash_transition, _transition_target> transitions_;
};

template <std_simple_allocator Alloc>
constexpr entity_t world_base<Alloc>::spawn() {
    return _get_new_entity();
}

template <std_simple_allocator Alloc>
constexpr auto world_base<Alloc>::spawn_n(size_type n) {
    return _entity_slots::_get_new_entities(n);
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr void world_base<Alloc>::_emplace_new_entity(entity_t entity) {
    using namespace neutron;
    using list              = type_list<Components...>;
    constexpr uint64_t hash = make_array_hash<list>();

    const auto index = _get_index(entity);
    auto& slot       = _entity_slot(index);
    auto iter        = archetypes_.find(hash);
    if (iter != archetypes_.end()) {
        iter->second.template emplace<Components...>(entity);
        slot.second = &iter->second;
    } else {
        auto [iter, _] = archetypes_.try_emplace(
            hash, archetype{ spread_type<Components...> });
        iter->second.template emplace<Components...>(entity);
        slot.second = &iter->second;
    }
}

template <std_simple_allocator Alloc>
template <component... Components, compatible_range<entity_t> Rng>
constexpr void world_base<Alloc>::_emplace_new_entities(Rng& range) {
    using namespace neutron;
    using list              = type_list<Components...>;
    constexpr uint64_t hash = make_array_hash<list>();

    auto bind_archetype = [this](archetype* target_archetype) noexcept {
        return [this, target_archetype](entity_t entity) noexcept {
            _entity_slot(_get_index(entity)).second = target_archetype;
        };
    };
    auto reset_archetype = [this](entity_t entity) noexcept {
        _entity_slot(_get_index(entity)).second = nullptr;
    };

    auto iter = archetypes_.find(hash);
    if (iter != archetypes_.end()) {
        auto on_append = bind_archetype(&iter->second);
        iter->second.template _emplace_with_entity_binding<Components...>(
            range, on_append, reset_archetype);
    } else {
        auto [created, _] = archetypes_.try_emplace(
            hash, archetype{ spread_type<Components...> });
        auto on_append = bind_archetype(&created->second);
        created->second.template _emplace_with_entity_binding<Components...>(
            range, on_append, reset_archetype);
    }
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr entity_t world_base<Alloc>::spawn() {
    const auto entity = _get_new_entity();
    _emplace_new_entity<Components...>(entity);
    return entity;
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr auto world_base<Alloc>::spawn_n(size_type n) {
    auto range = _entity_slots::_get_new_entities(n);
    if (range.empty()) {
        return range;
    }

    _emplace_new_entities<Components...>(range);
    return range;
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr void world_base<Alloc>::_emplace_new_entity(
    entity_t entity, Components&&... components) {
    using namespace neutron;
    using list              = type_list<std::remove_cvref_t<Components>...>;
    constexpr uint64_t hash = make_array_hash<list>();

    const auto index = _get_index(entity);
    auto& slot       = _entity_slot(index);
    auto iter        = archetypes_.find(hash);
    if (iter != archetypes_.end()) [[likely]] {
        iter->second.emplace(entity, std::forward<Components>(components)...);
        slot.second = &iter->second;
    } else [[unlikely]] {
        auto [iter, _] = archetypes_.try_emplace(
            hash, archetype{ spread_type<std::remove_cvref_t<Components>...> });
        iter->second.emplace(entity, std::forward<Components>(components)...);
        slot.second = &iter->second;
    }
}

template <std_simple_allocator Alloc>
template <component... Components, compatible_range<entity_t> Rng>
constexpr void world_base<Alloc>::_emplace_new_entities(
    Rng& range, Components&&... components) {
    using namespace neutron;
    using list              = type_list<std::remove_cvref_t<Components>...>;
    constexpr uint64_t hash = make_array_hash<list>();

    auto bind_archetype = [this](archetype* target_archetype) noexcept {
        return [this, target_archetype](entity_t entity) noexcept {
            _entity_slot(_get_index(entity)).second = target_archetype;
        };
    };
    auto reset_archetype = [this](entity_t entity) noexcept {
        _entity_slot(_get_index(entity)).second = nullptr;
    };

    auto iter = archetypes_.find(hash);
    if (iter != archetypes_.end()) [[likely]] {
        auto on_append = bind_archetype(&iter->second);
        iter->second._emplace_with_entity_binding(
            range, on_append, reset_archetype,
            std::forward<Components>(components)...);
    } else [[unlikely]] {
        auto [created, _] = archetypes_.try_emplace(
            hash, archetype{ spread_type<std::remove_cvref_t<Components>...> });
        auto on_append = bind_archetype(&created->second);
        created->second._emplace_with_entity_binding(
            range, on_append, reset_archetype,
            std::forward<Components>(components)...);
    }
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr entity_t world_base<Alloc>::spawn(Components&&... components) {
    const auto entity = _get_new_entity();
    _emplace_new_entity(entity, std::forward<Components>(components)...);
    return entity;
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr auto
    world_base<Alloc>::spawn_n(size_type n, Components&&... components) {
    auto range = _entity_slots::_get_new_entities(n);
    if (range.empty()) {
        return range;
    }

    _emplace_new_entities(range, std::forward<Components>(components)...);
    return range;
}

template <std_simple_allocator Alloc>
template <component... Components>
auto world_base<Alloc>::_dst_transition_add(
    archetype* archetype, uint64_t delta) -> _transition_target& {
    using tlist = type_list<std::remove_cvref_t<Components>...>;
    _hash_transition cond{
        .from  = archetype->hash(),
        .delta = delta,
        .op    = _transition_op::add,
    };
    if (auto trans = transitions_.find(cond); trans != transitions_.end())
        [[likely]] {
        return trans->second;
    }

    constexpr auto arr = make_hash_array<tlist>();
    size_t size        = archetype->hash_list().size() + arr.size();
    _vector_t<uint32_t> hash_list(archetypes_.get_allocator());
    hash_list.reserve(size);
    std::ranges::set_union(
        archetype->hash_list(), arr, std::back_inserter(hash_list));

    const auto hash = hash_combine(hash_list);
    auto* target    = hash == archetype->hash() ? archetype : nullptr;
    auto [trans, _] =
        transitions_.emplace(cond, _transition_target{ hash, target });
    return trans->second;
}

template <std_simple_allocator Alloc>
template <component... Components>
auto world_base<Alloc>::_dst_transition_remove(
    archetype* archetype, uint64_t delta) -> _transition_target& {
    using tlist = type_list<std::remove_cvref_t<Components>...>;
    _hash_transition cond{
        .from  = archetype->hash(),
        .delta = delta,
        .op    = _transition_op::remove,
    };
    if (auto trans = transitions_.find(cond); trans != transitions_.end())
        [[likely]] {
        return trans->second;
    }

    constexpr auto arr = make_hash_array<tlist>();
    _vector_t<uint32_t> hash_list(archetypes_.get_allocator());
    hash_list.reserve(archetype->hash_list().size());
    std::ranges::set_difference(
        archetype->hash_list(), arr, std::back_inserter(hash_list));

    const auto hash = hash_combine(hash_list);
    auto* target    = hash == archetype->hash() ? archetype : nullptr;
    auto [trans, _] =
        transitions_.emplace(cond, _transition_target{ hash, target });
    return trans->second;
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr void world_base<Alloc>::add_components(entity_t entity) {
    using namespace neutron;
    using tlist             = type_list<std::remove_cvref_t<Components>...>;
    constexpr uint64_t hash = make_array_hash<tlist>();

    const auto index           = _get_index(entity);
    auto& slot                 = _entity_slot(index);
    archetype* const archetype = slot.second;
    if (archetype == nullptr) {
        _emplace_new_entity<Components...>(entity);
        return;
    }

    // get dst hash
    auto& transition = _dst_transition_add<std::remove_cvref_t<Components>...>(
        archetype, hash);
    if (transition.target == archetype) [[unlikely]] {
        return;
    }

    auto* target = transition.target;
    if (target == nullptr) [[unlikely]] {
        auto iter = archetypes_.find(transition.hash);
        if (iter == archetypes_.end()) [[unlikely]] {
            auto [it, _] = archetypes_.try_emplace(
                transition.hash, *archetype,
                add_components_t<std::remove_cvref_t<Components>...>{});
            iter = it;
        }
        target            = &iter->second;
        transition.target = target;
    }

    // do move
    archetype->transfer(
        entity, *target,
        add_components_t<std::remove_cvref_t<Components>...>{});
    slot.second = target;
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr void world_base<Alloc>::add_components(
    entity_t entity, Components&&... components) {
    constexpr uint64_t hash = neutron::make_array_hash<
        neutron::type_list<std::remove_cvref_t<Components>...>>();
    const auto index      = _get_index(entity);
    auto& slot            = _entity_slot(index);
    auto* const archetype = slot.second;
    if (archetype == nullptr) {
        _emplace_new_entity<Components...>(
            entity, std::forward<Components>(components)...);
        return;
    }

    auto& transition = _dst_transition_add<std::remove_cvref_t<Components>...>(
        archetype, hash);
    if (transition.target == archetype) [[unlikely]] {
        return;
    }

    auto* target = transition.target;
    if (target == nullptr) [[unlikely]] {
        auto iter = archetypes_.find(transition.hash);
        if (iter == archetypes_.end()) [[unlikely]] {
            auto [it, _] = archetypes_.try_emplace(
                transition.hash, *archetype,
                add_components_t<std::remove_cvref_t<Components>...>{});
            iter = it;
        }
        target            = &iter->second;
        transition.target = target;
    }

    archetype->transfer(
        entity, *target, add_components_t<std::remove_cvref_t<Components>...>{},
        std::forward<Components>(components)...);
    slot.second = target;
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr void world_base<Alloc>::remove_components(entity_t entity) {
    using namespace neutron;
    using tlist             = type_list<std::remove_cvref_t<Components>...>;
    constexpr uint64_t hash = make_array_hash<tlist>();

    const auto index           = _get_index(entity);
    auto& slot                 = _entity_slot(index);
    archetype* const archetype = slot.second;
    if (archetype == nullptr) [[unlikely]] {
        return;
    }

    auto& transition = _dst_transition_remove<Components...>(archetype, hash);
    if (transition.target == archetype) [[unlikely]] {
        return;
    }

    if (transition.hash == empty_archetype_hash) {
        archetype->erase(entity);
        slot.second = nullptr;
        return;
    }

    auto* target = transition.target;
    if (target == nullptr) [[unlikely]] {
        auto iter = archetypes_.find(transition.hash);
        if (iter == archetypes_.end()) [[unlikely]] {
            auto [it, _] = archetypes_.try_emplace(
                transition.hash, *archetype,
                remove_components_t<Components...>{});
            iter = it;
        }
        target            = &iter->second;
        transition.target = target;
    }

    archetype->transfer(entity, *target);
    slot.second = target;
}

template <std_simple_allocator Alloc>
constexpr void world_base<Alloc>::kill(entity_t entity) {
    const auto index = _get_index(entity);
    auto& slot       = _entity_slot(index);
    assert(entity == slot.first);

    auto*& arche = slot.second;
    if (arche != nullptr) {
        arche->erase(entity);
        arche = nullptr;
    }
    _release_entity(index);
}

template <std_simple_allocator Alloc>
template <compatible_range<entity_t> Rng>
constexpr void world_base<Alloc>::kill(Rng&& range) {
    for (entity_t entity : range) {
        kill(entity);
    }
}

template <std_simple_allocator Alloc>
constexpr void world_base<Alloc>::reserve(size_type n) {
    _reserve_entities(n);
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr void world_base<Alloc>::reserve(size_type n) {
    using namespace neutron;
    using list          = type_list<Components...>;
    constexpr auto hash = make_array_hash<list>();

    if (auto iter = archetypes_.find(hash); iter != archetypes_.end()) {
        iter->second.reserve(n);
    } else {
        auto [it, succ] =
            archetypes_.emplace(hash, archetype{ spread_type<Components...> });
        it->second.reserve(n);
    }

    _reserve_entities(n);
}

template <std_simple_allocator Alloc>
constexpr bool world_base<Alloc>::is_alive(entity_t entity) noexcept {
    return _is_alive(entity);
}

template <std_simple_allocator Alloc>
void world_base<Alloc>::clear() {
    for (auto& [_, archetype] : archetypes_) {
        archetype.clear();
    }
    _clear_entities();
}

} // namespace _world_base

template <std_simple_allocator Alloc = std::allocator<std::byte>>
using world_base = _world_base::world_base<Alloc>;

} // namespace neutron

namespace std {

template <>
struct hash<neutron::_world_base::_hash_transition> {
    size_t operator()(const neutron::_world_base::_hash_transition& transition)
        const noexcept {
        return hash<uint64_t>{}(transition.from) ^
               (hash<uint64_t>{}(transition.delta) << 1) ^
               (hash<uint8_t>{}(static_cast<uint8_t>(transition.op)) << 2);
    }
};

} // namespace std
