// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>
#include <neutron/concepts.hpp>
#include <neutron/metafn.hpp>
#include "neutron/detail/ecs/archetype.hpp"
#include "neutron/detail/ecs/bundle.hpp"
#include "neutron/detail/ecs/slice.hpp"
#include "neutron/detail/ecs/world_accessor.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/type_traits/same_cvref.hpp"
#include "neutron/metafn.hpp"

namespace neutron {

template <component_like...>
struct with;
template <component_like...>
struct without;
template <component_like...>
struct withany;
template <component_like...>
struct changed;

template <component_like... Args>
struct with {
    using _with_t =
        type_list_recurse_expose_t<bundle, with<Args...>, neutron::same_cvref>;

    template <typename>
    struct _impl;
    template <component... Components>
    struct _impl<with<Components...>> {
        template <typename Archetype>
        static constexpr bool init(const Archetype& archetype) {
            return archetype.template has<std::remove_cvref_t<Components>...>();
        }
    };

    constexpr bool init(const auto& archetype) {
        return _impl<_with_t>::init(archetype);
    }
};

template <component_like... Args>
struct without {
    using _without_t = type_list_recurse_expose_t<
        bundle, without<Args...>, neutron::same_cvref>;

    template <typename>
    struct _impl;
    template <component... Components>
    struct _impl<without<Components...>> {
        template <typename Archetype>
        static constexpr void init(const Archetype& archetype) {
            return (
                !archetype.template has<std::remove_cvref_t<Components>>() &&
                ...);
        }
    };

    constexpr bool init(const auto& archetype) {
        return _impl<_without_t>::init(archetype);
    }
};

template <component_like... Args>
struct withany {
    using _withany_t = neutron::type_list_recurse_expose_t<
        bundle, withany<Args...>, neutron::same_cvref>;

    template <typename>
    struct _impl;
    template <component... Components>
    struct _impl<withany<Components...>> {
        template <typename Archetype>
        static constexpr bool init(const Archetype& archetype) {
            return (
                archetype.template has<std::remove_cvref_t<Components>>() ||
                ...);
        }
    };

    constexpr bool init(const auto& archetype) {
        return _impl<_withany_t>::init(archetype);
    }
};

template <component_like... Args>
struct changed {
    using conflict_list = without<Args...>;
    constexpr bool fetch(const auto& archetype) { return true; }
};

template <typename Ty>
struct _is_with_like {
    constexpr static auto value = is_specific_type_list_v<with, Ty> ||
                                  is_specific_type_list_v<without, Ty> ||
                                  is_specific_type_list_v<withany, Ty>;
};

template <typename Ty>
struct _is_with : is_specific_type_list<with, Ty> {};
template <typename Ty>
constexpr auto _is_with_v = _is_with<Ty>::value;

template <typename Ty>
struct _is_without : is_specific_type_list<without, Ty> {};
template <typename Ty>
constexpr auto _is_without_v = _is_without<Ty>::value;

template <typename Ty>
struct _is_withany : is_specific_type_list<withany, Ty> {};
template <typename Ty>
constexpr auto _is_withany_v = _is_withany<Ty>::value;

namespace _query_filter {

template <
    typename Filter, typename Alloc,
    typename Archetype = archetype<rebind_alloc_t<Alloc, std::byte>>>
concept _has_init = requires(Filter& filter, const Archetype& archetype) {
    { filter.init(archetype) } -> std::same_as<bool>;
};

template <
    typename Filter, typename Alloc,
    typename Archetype = archetype<rebind_alloc_t<Alloc, std::byte>>>
concept _has_fetch =
    requires(Filter& filter, const Archetype& archetype, entity_t entity) {
        { filter.fetch(archetype, entity) } -> std::same_as<bool>;
    };

} // namespace _query_filter

template <typename Filter, typename Alloc>
concept query_filter = _query_filter::_has_init<Filter, Alloc> ||
                       _query_filter::_has_fetch<Filter, Alloc>;

template <typename T>
using _not_empty = std::negation<std::is_empty<T>>;

namespace _basic_querior {

template <std_simple_allocator Alloc, typename... Filters>
struct _basic_querior_base {

    static_assert(
        (query_filter<Filters, Alloc> && ...),
        "basic_querior filters must satisfy query_filter");

    template <typename Ty>
    using _allocator_t = rebind_alloc_t<Alloc, Ty>;

    template <typename Ty>
    using _vector_t = std::vector<Ty, _allocator_t<Ty>>;

    using _archetype_t = archetype<_allocator_t<std::byte>>;

    template <typename Filter>
    struct _has_init :
        std::bool_constant<_query_filter::_has_init<
            Filter, _allocator_t<std::byte>, _archetype_t>> {};

    template <typename Filter>
    struct _has_fetch :
        std::bool_constant<_query_filter::_has_fetch<
            Filter, _allocator_t<std::byte>, _archetype_t>> {};

    using filters_type   = type_list<Filters...>;
    using component_list = type_list_recurse_expose_t<
        bundle,
        type_list_expose_t<
            with, type_list_filt_t<_is_with, type_list<Filters...>>>,
        same_cvref>;
    using initable_filters  = type_list_filt_t<_has_init, filters_type>;
    using fetchable_filters = type_list_filt_t<_has_fetch, filters_type>;
    using nempty_comp_list  = type_list_filt_t<_not_empty, component_list>;
    using rnempty_comp_list =
        type_list_convert_t<std::remove_cvref, nempty_comp_list>;
    using slice_t        = type_list_rebind_t<slice, rnempty_comp_list>;
    using allocator_type = _allocator_t<slice_t>;
    using value_type     = slice_t;

    _basic_querior_base() = default;

    template <typename Al = Alloc>
    _basic_querior_base(const Al& alloc) : slices(alloc) {}

    template <typename... Flt>
    ATOM_NODISCARD static bool
        init(type_list<Flt...>, const _archetype_t& archetype) noexcept {
        return (Flt{}.init(archetype) && ...);
    }

    template <typename... Flt>
    ATOM_NODISCARD static bool fetch(
        type_list<Flt...>, const _archetype_t& archetype,
        entity_t entity) noexcept {
        return (Flt{}.fetch(archetype, entity) && ...);
    }

    void make(auto& archetypes) {
        slices.clear();
        for (auto& [_, archetype] : archetypes) {
            if (init(initable_filters{}, archetype)) {
                slices.emplace_back(archetype);
            }
        }
    }

    _vector_t<slice_t> slices;
};

template <std_simple_allocator Alloc, bool AutoConstruct, typename... Filters>
class basic_querior : private _basic_querior_base<Alloc, Filters...> {
    using _base = _basic_querior_base<Alloc, Filters...>;

public:
    using typename _base::initable_filters;
    using typename _base::fetchable_filters;
    using typename _base::component_list;

    static_assert(
        (query_filter<Filters, Alloc> && ...),
        "basic_querior filters must satisfy query_filter");

    basic_querior() = default;

    template <world World>
    explicit basic_querior(World& world) : _base(world.get_allocator()) {
        sync(world);
    }

    template <world World>
    void sync(World& world) {
        const auto ver = world_accessor::version(world);
        if (ver == version_) [[likely]] {
            return;
        }

        auto& archetypes = world_accessor::archetypes(world);
        this->make(archetypes);
        version_ = ver;
    }

    ATOM_NODISCARD auto slices() const noexcept
        -> std::span<const typename _base::slice_t> {
        return _base::slices;
    }

    ATOM_NODISCARD size_t size() const noexcept { return _base::slices.size(); }

    ATOM_NODISCARD size_t version() const noexcept { return version_; }

private:
    size_t version_ = 0;
};

template <typename Alloc, query_filter<Alloc>... Filters>
class basic_querior<Alloc, false, Filters...> :
    private _basic_querior_base<Alloc, Filters...> {
    using _base = _basic_querior_base<Alloc, Filters...>;

public:
    using typename _base::initable_filters;
    using typename _base::fetchable_filters;
    using typename _base::component_list;

    template <world World>
    basic_querior(World& world) {
        auto& archetypes = world_accessor::archetypes(world);
        this->make(archetypes);
    }

    ATOM_NODISCARD auto slices() const noexcept
        -> std::span<const typename _base::slice_t> {
        return _base::slices;
    }

    ATOM_NODISCARD size_t size() const noexcept { return _base::slices.size(); }
};

} // namespace _basic_querior

template <std_simple_allocator Alloc, typename... Filters>
using basic_querior = _basic_querior::basic_querior<Alloc, false, Filters...>;

namespace internal {
template <typename Query>
struct is_querior : std::false_type {};
template <typename Alloc, bool AutoConstruct, typename... Filters>
struct is_querior<
    _basic_querior::basic_querior<Alloc, AutoConstruct, Filters...>> :
    std::true_type {};
} // namespace internal

} // namespace neutron
