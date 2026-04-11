// private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>
#include "neutron/detail/ecs/basic_commands.hpp" // IWYU pragma: keep
#include "neutron/detail/ecs/construct_from_world.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/metafn/rebind.hpp"
#include "neutron/reflection.hpp"

namespace neutron {

class typeinfo {
    constexpr typeinfo(uint32_t hash, std::string_view name) noexcept
        : hash_(hash), name_(name) {}

public:
    ATOM_NODISCARD constexpr uint32_t hash() const noexcept { return hash_; }

    ATOM_NODISCARD constexpr std::string_view name() const noexcept {
        return name_;
    }

    template <typename Ty>
    static constexpr typeinfo make() noexcept {
        return typeinfo{ hash_of<Ty>(), name_of<Ty>() };
    }

    constexpr bool operator==(uint32_t hash) const noexcept {
        return hash_ == hash;
    }

    constexpr bool operator==(std::string_view name) const noexcept {
        return name_ == name;
    }

private:
    uint32_t hash_;
    std::string_view name_;
};

template <typename Alloc = std::allocator<std::byte>>
class basic_meta_accessor {
    using _iter_t = const typeinfo*;

public:
    template <typename Ret, typename... Args>
    using _sfn_t = Ret (*)(Args...) noexcept;
    template <typename Ret, typename... Args>
    using _fn_t        = Ret (*)(Args...);
    using _begin_fn    = _sfn_t<_iter_t>;
    using _end_fn      = _sfn_t<_iter_t>;
    using _findhash_fn = _sfn_t<_iter_t, uint32_t>;
    using _findname_fn = _sfn_t<_iter_t, std::string_view>;
    using _add_fn      = _fn_t<bool, basic_commands<Alloc>, entity_t, _iter_t>;
    using _rm_fn       = _fn_t<bool, basic_commands<Alloc>, entity_t, _iter_t>;

    explicit basic_meta_accessor(
        // NOLINTNEXTLINE
        _begin_fn begin, _end_fn end, _findhash_fn findhash,
        _findname_fn findname, _add_fn add, _rm_fn rm) noexcept
        : begin_(begin), end_(end), findhash_(findhash), findname_(findname),
          add_component_(add), remove_component_(rm) {}

    ATOM_NODISCARD _iter_t begin() const noexcept { return begin_(); }

    ATOM_NODISCARD _iter_t end() const noexcept { return end_(); }

    ATOM_NODISCARD _iter_t find(uint32_t hash) const noexcept {
        return findhash_(hash);
    }

    ATOM_NODISCARD _iter_t find(std::string_view name) const noexcept {
        return findname_(name);
    }

    bool add_component(
        basic_commands<Alloc> cmds, entity_t entity, _iter_t iter) const {
        return add_component_(cmds, entity, iter);
    }

    bool remove_component(
        basic_commands<Alloc> cmds, entity_t entity, _iter_t iter) const {
        return remove_component_(cmds, entity, iter);
    }

private:
    _begin_fn begin_;
    _end_fn end_;
    _findhash_fn findhash_;
    _findname_fn findname_;
    _add_fn add_component_;
    _rm_fn remove_component_;
};

namespace _meta_accessor {

template <typename Alloc, typename... Components>
class _accessor : public basic_meta_accessor<Alloc> {
    static constexpr std::array<typeinfo, sizeof...(Components)> info = {
        typeinfo::make<Components>()...
    };

public:
    ATOM_NODISCARD static constexpr const typeinfo* begin() noexcept {
        return info.data();
    }

    ATOM_NODISCARD static constexpr const typeinfo* end() noexcept {
        return info.data() + info.size();
    }

    ATOM_NODISCARD static const typeinfo* findhash(uint32_t hash) noexcept {
        return std::find(begin(), end(), hash);
    }

    ATOM_NODISCARD static const typeinfo*
        findname(std::string_view name) noexcept {
        return std::find(begin(), end(), name);
    }

    static bool add_component(
        basic_commands<Alloc> cmds, entity_t entity, const typeinfo* iter) {
        size_t index = std::distance(begin(), iter);
        return [cmds, entity, index]<size_t... Is>(std::index_sequence<Is...>) {
            return (_add_component<Is>(cmds, entity, index) || ...);
        }(std::index_sequence_for<Components...>());
    }

    static bool remove_component(
        basic_commands<Alloc> cmds, entity_t entity, const typeinfo* iter) {
        size_t index = std::distance(begin(), iter);
        return [cmds, entity, index]<size_t... Is>(std::index_sequence<Is...>) {
            return (_remove_component<Is>(cmds, entity, index) || ...);
        }(std::index_sequence_for<Components...>());
    }

private:
    template <size_t Index>
    static bool _add_component(
        // NOLINTNEXTLINE
        basic_commands<Alloc> cmds, entity_t entity, size_t index) {
        using type = type_list_element_t<Index, _accessor<Components...>>;
        if (index == Index) {
            cmds.template add_components<type>(entity);
            return true;
        }
        return false;
    }

    template <size_t Index>
    static bool _remove_component(
        // NOLINTNEXTLINE
        basic_commands<Alloc> cmds, entity_t entity, size_t index) {
        using type = type_list_element_t<Index, _accessor<Components...>>;
        if (index == Index) {
            cmds.template remove_components<type>(entity);
            return true;
        }
        return false;
    }
};

} // namespace _meta_accessor

template <auto Sys, size_t Index, typename Alloc>
struct construct_from_world_t<Sys, basic_meta_accessor<Alloc>, Index> {
    template <typename... Components>
    using _accessor_t = _meta_accessor::_accessor<Alloc, Components...>;

    template <world World>
    basic_meta_accessor<Alloc> operator()(World& world) const {
        using components = typename std::remove_cvref_t<World>::components;
        using accessor_t = type_list_rebind_t<_accessor_t, components>;
        return basic_meta_accessor<Alloc>{
            &accessor_t::begin,         &accessor_t::end,
            &accessor_t::findhash,      &accessor_t::findname,
            &accessor_t::add_component, &accessor_t::remove_component
        };
    }
};

using meta_accessor = basic_meta_accessor<std::allocator<std::byte>>;

} // namespace neutron
