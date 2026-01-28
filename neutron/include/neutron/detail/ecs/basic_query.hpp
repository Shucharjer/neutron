#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <concepts>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include "neutron/detail/ecs/archetype.hpp"
#include "neutron/detail/ecs/bundle.hpp"
#include "neutron/detail/ecs/world_accessor.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/type_traits/same_cvref.hpp"
#include "neutron/metafn.hpp"
#include "neutron/smvec.hpp"

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

namespace internal {} // namespace internal

template <
    typename Filter, typename Alloc,
    typename Archetype = archetype<rebind_alloc_t<Alloc, std::byte>>>
concept query_filter = _query_filter::_has_init<Filter, Alloc, Archetype> ||
                       _query_filter::_has_fetch<Filter, Alloc, Archetype>;

template <
    std_simple_allocator Alloc, size_t Count, query_filter<Alloc>... Filters>
class basic_query<Alloc, Count, Filters...> {
    template <typename Ty>
    using _allocator_t = rebind_alloc_t<Alloc, Ty>;

    template <typename Ty>
    using _vector_t = neutron::smvec<Ty, 4, _allocator_t<Ty>>;

    using _archetype_t = archetype<_allocator_t<std::byte>>;

    template <typename... Tys>
    using _query_t = basic_query<Alloc, Count, Tys...>;

    template <typename Filter>
    struct _has_init :
        std::bool_constant<_query_filter::_has_init<
            Filter, _allocator_t<std::byte>, _archetype_t>> {};

    template <typename Filter>
    struct _has_fetch :
        std::bool_constant<_query_filter::_has_fetch<
            Filter, _allocator_t<std::byte>, _archetype_t>> {};

public:
// The implementation of `withany` might be somewhat complex, so it will be put
// on hold for now.
// NOLINTNEXTLINE
#if false
    template <typename Filter>
    using _is_with_or_withany =
        std::bool_constant<_is_with_v<Filter> || _is_withany_v<Filter>>;
#else
    template <typename Filter>
    using _is_with_or_withany = std::bool_constant<_is_with_v<Filter>>;
#endif

    using filters_type   = type_list<Filters...>;
    using component_list = type_list_recurse_expose_t<
        bundle,
        type_list_expose_t<with, type_list_filt_t<_is_with, filters_type>>,
        same_cvref>;
    using initable_filters  = type_list_filt_t<_has_init, filters_type>;
    using fetchable_filters = type_list_filt_t<_has_fetch, filters_type>;

    using view_t  = type_list_rebind_t<view, component_list>;
    using eview_t = type_list_rebind_t<eview, component_list>;

    template <world World>
    explicit basic_query(World& world) {
        auto& archetypes = world_accessor::archetypes(world);
        for (auto& [hash, archetype] : archetypes) {
            if (_init(initable_filters{}, archetype)) {
                archetypes_.emplace_back(&archetype);
            }
        }
    }

    ATOM_NODISCARD auto get() noexcept {
        return archetypes_ |
               std::views::transform([](_archetype_t* archetype) -> view_t {
                   return view_t(*archetype);
               }) |
               std::views::join;
    }

    template <component Component>
    ATOM_NODISCARD auto get() noexcept {
        return archetypes_ | std::views::filter([](_archetype_t* archetype) {
                   return archetype->template has<Component>();
               }) |
               std::views::transform(
                   [](_archetype_t* archetype) -> ::neutron::view<Component> {
                       return ::neutron::view<Component>(*archetype);
                   }) |
               std::views::join;
    }

    ATOM_NODISCARD auto get_with_entity() noexcept {
        return archetypes_ |
               std::views::transform([](_archetype_t* archetype) -> eview_t {
                   return eview_t(*archetype);
               }) |
               std::views::join;
    }

    template <component Component>
    ATOM_NODISCARD auto get_with_entity() noexcept {
        return archetypes_ | std::views::filter([](_archetype_t* archetype) {
                   return archetype->template has<Component>();
               }) |
               std::views::transform(
                   [](_archetype_t* archetype) -> ::neutron::eview<Component> {
                       return ::neutron::eview<Component>(*archetype);
                   }) |
               std::views::join;
    }

    ATOM_NODISCARD auto entities() noexcept {
        return archetypes_ |
               std::views::transform([](_archetype_t* archetype) -> view_t {
                   return eview_t{ *archetype }.entities();
               }) |
               std::views::join;
    }

    ATOM_NODISCARD auto raw() noexcept -> _vector_t<_archetype_t*> {
        return archetypes_;
    }

    ATOM_NODISCARD size_t size() const noexcept { return archetypes_.size(); }

private:
    template <typename... Flt>
    ATOM_NODISCARD static bool
        _init(type_list<Flt...>, const _archetype_t& archetype) noexcept {
        return (Flt{}.init(archetype) && ...);
    }

    template <typename... Flt>
    ATOM_NODISCARD static bool _fetch(
        type_list<Flt...>, const _archetype_t& archetype,
        entity_t entity) noexcept {
        return (Flt{}.fetch(archetype, entity) && ...);
    }

    _vector_t<_archetype_t*> archetypes_;
};

namespace internal {
template <typename Query>
struct is_query : std::false_type {};
template <typename Alloc, size_t Count, query_filter<Alloc>... Filters>
struct is_query<basic_query<Alloc, Count, Filters...>> : std::true_type {};
} // namespace internal

} // namespace neutron
