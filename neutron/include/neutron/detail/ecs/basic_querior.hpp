#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <concepts>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include "neutron/detail/ecs/archetype.hpp"
#include "neutron/detail/ecs/bundle.hpp"
#include "neutron/detail/ecs/world_accessor.hpp"
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

    constexpr bool init(auto& archetypes) {
        return _impl<_with_t>::init(archetypes);
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

    constexpr bool init(const auto& archetypes) {
        return _impl<_without_t>::init(archetypes);
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

    constexpr bool init(auto& archetypes) {
        return _impl<_withany_t>::init(archetypes);
    }
};

template <component_like... Args>
struct changed {
    using conflict_list = without<Args...>;
    template <typename Archetype>
    constexpr bool init(const Archetype& archetype) {
        return true;
    }
    constexpr bool fetch(auto& out, const auto& archetype) { return true; }
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
concept _initable_filter = requires(const Archetype& archetype) {
    { Filter{}.init(archetype) } -> std::same_as<bool>;
};

template <
    typename Filter, typename Alloc,
    typename Archetype = archetype<rebind_alloc_t<Alloc, std::byte>>>
concept _fetchable_filter = requires(const Archetype& archetype) {
    { Filter{}.fetch(archetype) } -> std::same_as<bool>;
};

} // namespace _query_filter

template <typename Filter, typename Alloc>
concept query_filter = _query_filter::_initable_filter<Filter, Alloc> ||
                       _query_filter::_fetchable_filter<Filter, Alloc>;

template <
    std_simple_allocator Alloc, size_t Count, query_filter<Alloc>... Filters>
class basic_querior<Alloc, Count, Filters...> {
    template <typename Ty>
    using _allocator_t = rebind_alloc_t<Alloc, Ty>;

    template <typename Ty>
    using _vector_t = neutron::smvec<Ty, 4, _allocator_t<Ty>>;

    using _archetype_t = archetype<_allocator_t<std::byte>>;

    template <component... Components>
    using _view_type = view<_allocator_t<std::byte>, Components...>;

    template <component... Components>
    using _eview_type = eview<_allocator_t<std::byte>, Components...>;

    template <typename... Tys>
    using _query_t = basic_querior<Alloc, Count, Tys...>;

public:
    using component_list = type_list_recurse_expose_t<
        bundle,
        type_list_expose_t<with, type_list_filt_t<_is_with, basic_querior>>,
        same_cvref>;

    using view_t  = type_list_rebind_t<_view_type, component_list>;
    using eview_t = type_list_rebind_t<_eview_type, component_list>;

    template <world World>
    explicit basic_querior(World& world) {
        auto& archetypes = world_accessor::archetypes(world);
        for (auto& [hash, archetype] : archetypes) {
            if ((Filters{}.init(archetype) && ...)) {
                archetypes_.emplace_back(&archetype);
            }
        }
    }

    auto get() noexcept {
        return archetypes_ | std::views::transform([](_archetype_t* archetype) {
                   return view_of(*archetype, component_list{});
               }) |
               std::views::join;
    }

    auto get_with_entity() noexcept {
        return archetypes_ | std::views::transform([](_archetype_t* archetype) {
                   // TODO: something like zip view
                   return eview_of(*archetype, component_list{});
               }) |
               std::views::join;
    }

    auto entities() noexcept {
        return archetypes_ | std::views::transform([](_archetype_t* archetype) {
                   return archetype->entities();
               }) |
               std::views::join;
    }

    ATOM_NODISCARD size_t size() const noexcept { return archetypes_.size(); }

private:
    _vector_t<_archetype_t*> archetypes_;
};

namespace internal {
template <typename Query>
struct is_querior : std::false_type {};
template <typename Alloc, size_t Count, query_filter<Alloc>... Filters>
struct is_querior<basic_querior<Alloc, Count, Filters...>> : std::true_type {};
} // namespace internal

} // namespace neutron
