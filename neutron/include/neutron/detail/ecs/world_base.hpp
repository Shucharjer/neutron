// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <queue>
#include <type_traits>
#include <utility>
#include <vector>
#include "neutron/detail/ecs/archetype.hpp"
#include "neutron/detail/ecs/component.hpp"
#include "neutron/flat_hash_map.hpp"
#include "neutron/memory.hpp"
#include "neutron/metafn.hpp"
#include "neutron/shift_map.hpp"
#include "neutron/small_vector.hpp"
#include "neutron/type_hash.hpp"
#include "neutron/utility.hpp"

namespace neutron {

struct world_accessor;

namespace _world_base {

struct _hash_transition {
    uint64_t from;
    uint64_t delta;

    constexpr bool operator==(const _hash_transition& that) const noexcept {
        return from == that.from && delta == that.delta;
    }
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
template <std_simple_allocator Alloc = std::allocator<std::byte>>
class world_base {
    template <typename, std_simple_allocator>
    friend class basic_world;

    friend struct ::neutron::world_accessor;

    template <typename Ty>
    using _allocator_t = neutron::rebind_alloc_t<Alloc, Ty>;

    template <typename Ty>
    using _vector_t = std::vector<Ty, _allocator_t<Ty>>;

    using archetype = archetype<Alloc>;

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

    template <typename Ty>
    using _priority_queue = std::priority_queue<Ty, _vector_t<Ty>>;

public:
    using size_type = size_t;

    template <typename Al = Alloc>
    explicit world_base(const Al& alloc = Alloc{})
        : archetypes_(alloc), entities_(1, alloc), transitions_(alloc) {}

    constexpr entity_t spawn();

    template <component... Components>
    constexpr entity_t spawn();

    template <component... Components>
    constexpr entity_t spawn(Components&&... components);

    template <component... Components>
    constexpr void add_components(entity_t entity);

    template <component... Components>
    constexpr void add_components(entity_t entity, Components&&... components);

    template <component... Components>
    constexpr void remove_components(entity_t entity);

    constexpr void kill(entity_t entity);

    constexpr void reserve(size_type n);

    template <component... Components>
    constexpr void reserve(size_type n);

    constexpr bool is_alive(entity_t entity) noexcept;

    void clear();

private:
    constexpr entity_t _get_new_entity();
    template <component... Components>
    constexpr void _emplace_new_entity(entity_t entity);
    template <component... Components>
    constexpr void _emplace_new_entity(entity_t entity, Components&&...);

    /// @brief A container stores archetypes with combined hash.
    archetype_map archetypes_;
    /// @brief A stroage mapping index to entity and its archetype.
    /// We do never pop or erase: when killing a entity, we set index zero
    /// and its archetype pointer null.
    /// Summary: for an entity ((gen, index), p_archetype),
    /// ((gen, non-zero), nullptr) -> created, but has no component
    /// ((gen, 0), 0) -> killed entity
    _vector_t<std::pair<entity_t, archetype*>> entities_;
    /// @brief A priority queue stores free indices.
    /// It makes us have the ability to get the smallest index all the time.
    /// A smaller index means we could access `sparse_map` or `shift_map` with
    /// lower frequency of cache missing.
    _priority_queue<uint32_t> free_indices_;

    /// @brief Cache for O(1) entity movement.
    _flat_hash_map<_hash_transition, uint64_t> transitions_;
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
constexpr entity_t world_base<Alloc>::_get_new_entity() {
    if (free_indices_.empty()) {
        const auto index = entities_.size();
        entities_.emplace_back(index, nullptr); // _make_entity(0, index)
        return index;
    }
    const index_t index = free_indices_.top();
    free_indices_.pop();
    const generation_t gen = _get_gen(entities_[index].first) + 1;
    entities_[index].first = _make_entity(gen, index);
    return entities_[index].first;
}

template <std_simple_allocator Alloc>
constexpr entity_t world_base<Alloc>::spawn() {
    return _get_new_entity();
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr void world_base<Alloc>::_emplace_new_entity(entity_t entity) {
    using namespace neutron;
    using list              = type_list<Components...>;
    constexpr uint64_t hash = make_array_hash<list>();

    const auto index = _get_index(entity);
    auto iter        = archetypes_.find(hash);
    if (iter != archetypes_.end()) {
        iter->second.template emplace<Components...>(entity);
        entities_[index].second = &iter->second;
    } else {
        auto [iter, _] = archetypes_.try_emplace(
            hash, archetype{ spread_type<Components...> });
        iter->second.template emplace<Components...>(entity);
        entities_[index].second = &iter->second;
    }
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr entity_t world_base<Alloc>::spawn() {
    constexpr uint64_t hash =
        neutron::make_array_hash<neutron::type_list<Components...>>();
    const auto entity = _get_new_entity();
    _emplace_new_entity<Components...>(entity);
    return entity;
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr void world_base<Alloc>::_emplace_new_entity(
    entity_t entity, Components&&... components) {
    using namespace neutron;
    using list              = type_list<std::remove_cvref_t<Components>...>;
    constexpr uint64_t hash = make_array_hash<list>();

    const auto index = _get_index(entity);
    auto iter        = archetypes_.find(hash);
    if (iter != archetypes_.end()) [[likely]] {
        iter->second.emplace(entity, std::forward<Components>(components)...);
        entities_[index].second = &iter->second;
    } else [[unlikely]] {
        auto [iter, _] = archetypes_.try_emplace(
            hash, archetype{ spread_type<std::remove_cvref_t<Components>...> });
        iter->second.emplace(entity, std::forward<Components>(components)...);
        entities_[index].second = &iter->second;
    }
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr entity_t world_base<Alloc>::spawn(Components&&... components) {
    constexpr uint64_t hash = neutron::make_array_hash<
        neutron::type_list<std::remove_cvref_t<Components>...>>();
    const auto entity = _get_new_entity();
    _emplace_new_entity(entity, std::forward<Components>(components)...);
    return entity;
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr void world_base<Alloc>::add_components(entity_t entity) {
    using namespace neutron;
    using tlist             = type_list<std::remove_cvref_t<Components>...>;
    constexpr uint64_t hash = make_array_hash<tlist>();

    const auto index           = _get_index(entity);
    archetype* const archetype = entities_[index].second;
    if (archetype == nullptr) {
        _emplace_new_entity<Components...>(entity);
        return;
    }

    // get dst hash
    _hash_transition cond{ .from = archetype->hash(), .delta = hash };
    uint64_t to = 0;
    if (auto trans = transitions_.find(cond); trans != transitions_.end())
        [[likely]] {
        to = trans->second;
    } else [[unlikely]] {
        constexpr auto arr = make_hash_array<tlist>();
        size_t size        = archetype->hash_list().size() + arr.size();
        _vector_t<uint32_t> hash_list(archetypes_.get_allocator());
        hash_list.reserve(size);
        std::ranges::merge(
            archetype->hash_list(), arr, std::back_inserter(hash_list));
        to = hash_combine(hash_list);
        transitions_.emplace_hint(trans, cond, to);
        transitions_.emplace(
            _hash_transition{ .from = to, .delta = hash }, archetype->hash());
    }

    // get target archetype by given dst hash
    auto iter = archetypes_.find(to);
    if (iter == archetypes_.end()) [[unlikely]] {
        auto [it, _] = archetypes_.try_emplace(
            to, *archetype, add_components_t<Components...>());
        iter = it;
    }

    // do move
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr void world_base<Alloc>::add_components(
    entity_t entity, Components&&... components) {
    constexpr uint64_t hash =
        neutron::make_array_hash<neutron::type_list<Components...>>();
    const auto index      = _get_index(entity);
    auto* const archetype = entities_[index].second;
    if (archetype == nullptr) {
        _emplace_new_entity<Components...>(
            entity, std::forward<Components>(components)...);
        return;
    }

    // move a entity from an archetype to another
}

template <std_simple_allocator Alloc>
template <component... Components>
constexpr void world_base<Alloc>::remove_components(entity_t entity) {
    using namespace neutron;
    using tlist             = type_list<std::remove_cvref_t<Components>...>;
    constexpr uint64_t hash = make_array_hash<tlist>();

    const auto index           = _get_index(entity);
    archetype* const archetype = entities_[index].second;
    if (archetype == nullptr) [[unlikely]] {
        return;
    }

    _hash_transition cond{ .from = archetype->hash(), .delta = hash };
    uint64_t to = 0;
    if (auto trans = transitions_.find(cond); trans != transitions_.end())
        [[likely]] {
        to = trans->second;
    } else [[unlikely]] {
        constexpr auto arr = make_hash_array<tlist>();
        size_t size        = archetype->hash_list().size() - arr.size();
        _vector_t<uint32_t> hash_list(archetypes_.get_allocator());
        hash_list.reserve(size);
        std::ranges::set_difference(
            archetype->hash_list(), arr, std::back_inserter(hash_list));
        to = hash_combine(hash_list);
        transitions_.emplace_hint(trans, cond, to);
        transitions_.emplace(
            _hash_transition{ .from = to, .delta = hash }, archetype->hash());
    }

    auto iter = archetypes_.find(to);
    if (iter == archetypes_.end()) [[unlikely]] {
        auto [it, _] = archetypes_.try_emplace(
            to, *archetype, add_components_t<Components...>{});
        iter = it;
    }

    // do move
}

template <std_simple_allocator Alloc>
constexpr void world_base<Alloc>::kill(entity_t entity) {
    const auto index = _get_index(entity);
    const auto gen   = _get_gen(entity);
    assert(entity == entities_[index].first);

    auto*& arche = entities_[index].second;
    if (arche != nullptr) {
        arche->erase(entity);
        arche = nullptr;
    }
    _reset_index(entities_[index].first);
    if (gen != (std::numeric_limits<uint32_t>::max)()) [[likely]] {
        free_indices_.push(index);
    }
}

template <std_simple_allocator Alloc>
constexpr void world_base<Alloc>::reserve(size_type n) {
    entities_.reserve(n);
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

    entities_.reserve(n);
}

template <std_simple_allocator Alloc>
constexpr bool world_base<Alloc>::is_alive(entity_t entity) noexcept {
    const auto index = _get_index(entity);
    return index != 0 && entities_.size() > index;
}

template <std_simple_allocator Alloc>
void world_base<Alloc>::clear() {
    for (auto& [_, archetype] : archetypes_) {
        archetype.clear();
    }
    free_indices_ = _priority_queue<index_t>{ entities_.get_allocator() };
    entities_.clear();
    entities_.emplace_back();
}

} // namespace _world_base

template <std_simple_allocator Alloc = std::allocator<std::byte>>
using world_base = _world_base::world_base<Alloc>;

} // namespace neutron

namespace std {

template <>
struct hash<neutron::_world_base::_hash_transition> {
    constexpr size_t operator()(const neutron::_world_base::_hash_transition&
                                    transition) const noexcept {
        return hash<uint64_t>{}(transition.from) ^
               (hash<uint64_t>{}(transition.delta) << 1);
    }
};

} // namespace std
