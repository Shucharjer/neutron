// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <cstddef>
#include <memory>
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>
#include "neutron/detail/ecs/basic_querior.hpp"
#include "neutron/detail/ecs/construct_from_world.hpp"
#include "neutron/detail/ecs/world_accessor.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/detail/metafn/convert.hpp"
#include "neutron/detail/tuple/shared_tuple.hpp"
#include "neutron/detail/type_traits/same_cvref.hpp"

namespace neutron {

namespace _query {

template <auto Sys, typename... Caches>
class query_tuple : public neutron::shared_tuple<Caches...> {
public:
    using _base_type = neutron::shared_tuple<Caches...>;

    using _base_type::_base_type;

    template <typename Cache>
    ATOM_NODISCARD constexpr auto get_first() & noexcept -> Cache& {
        return neutron::get_first<Cache>(static_cast<_base_type&>(*this));
    }

    template <typename Cache>
    ATOM_NODISCARD constexpr auto get_first() const& noexcept -> const Cache& {
        return neutron::get_first<Cache>(static_cast<const _base_type&>(*this));
    }
};

template <typename>
struct _storage_filter;

template <component_like... Args>
struct _storage_filter<with<Args...>> {
    using _expanded =
        type_list_recurse_expose_t<bundle, with<Args...>, neutron::same_cvref>;

    template <typename>
    struct _impl;
    template <component... Components>
    struct _impl<with<Components...>> {
        using type = with<std::remove_cvref_t<Components>...>;
    };

    using type = typename _impl<_expanded>::type;
};

template <component_like... Args>
struct _storage_filter<without<Args...>> {
    using _expanded = type_list_recurse_expose_t<
        bundle, without<Args...>, neutron::same_cvref>;

    template <typename>
    struct _impl;
    template <component... Components>
    struct _impl<without<Components...>> {
        using type = without<std::remove_cvref_t<Components>...>;
    };

    using type = typename _impl<_expanded>::type;
};

template <component_like... Args>
struct _storage_filter<withany<Args...>> {
    using _expanded = type_list_recurse_expose_t<
        bundle, withany<Args...>, neutron::same_cvref>;

    template <typename>
    struct _impl;
    template <component... Components>
    struct _impl<withany<Components...>> {
        using type = withany<std::remove_cvref_t<Components>...>;
    };

    using type = typename _impl<_expanded>::type;
};

template <component_like... Args>
struct _storage_filter<changed<Args...>> {
    using _expanded = type_list_recurse_expose_t<
        bundle, changed<Args...>, neutron::same_cvref>;

    template <typename>
    struct _impl;
    template <component... Components>
    struct _impl<changed<Components...>> {
        using type = changed<std::remove_cvref_t<Components>...>;
    };

    using type = typename _impl<_expanded>::type;
};

// template <std_simple_allocator Alloc, typename... Filters>
// struct to_manual_querior {
//     using type =
//         basic_querior<Alloc, false, typename
//         _storage_filter<Filters>::type...>;
// };

// template <std_simple_allocator Alloc, typename... Filters>
// struct to_cached_querior {
//     using type =
//         basic_querior<Alloc, true, typename
//         _storage_filter<Filters>::type...>;
// };

} // namespace _query

namespace internal {

template <typename>
struct is_query : std::false_type {};

template <typename... Filters>
struct is_query<query<Filters...>> : std::true_type {};

template <auto Sys, typename>
struct _is_relevant_query_tuple : std::false_type {};

template <auto Sys, typename... Caches>
struct _is_relevant_query_tuple<Sys, _query::query_tuple<Sys, Caches...>> :
    std::true_type {};

template <typename>
struct _is_empty_query_tuple : std::false_type {};

template <auto Sys>
struct _is_empty_query_tuple<_query::query_tuple<Sys>> : std::true_type {};

} // namespace internal

template <component... Components>
struct _view_base {
    _view_base(const slice<std::remove_cvref_t<Components>...>& slice) noexcept
        : size(slice.size()) {
        [this, &slice]<size_t... Is>(std::index_sequence<Is...>) {
            const auto& buffers = slice.buffers();
            bufs = tuple_t{ reinterpret_cast<std::remove_cvref_t<Components>*>(
                std::get<Is>(buffers)->get())... };
        }(std::index_sequence_for<Components...>());
    }
    using tuple_t = std::tuple<std::remove_cvref_t<Components>*...>;

    size_t size;
    tuple_t bufs;
};

template <component... Components>
class view : private _view_base<Components...> {
    using _base = _view_base<Components...>;
    using typename _base::tuple_t;

public:
    using value_type = std::tuple<Components...>;

    view(const slice<std::remove_cvref_t<Components>...>& slice) noexcept
        : _view_base<Components...>(slice) {}

    class iterator {
        friend class view;
        iterator(const tuple_t& comps) noexcept : comps_(comps) {}

    public:
        using value_type       = std::tuple<Components...>;
        using reference_type   = value_type;
        using size_type        = size_t;
        using difference_type  = ptrdiff_t;
        using iterator_concept = std::contiguous_iterator_tag;

        constexpr auto operator*() const noexcept -> std::tuple<Components...> {
            return std::apply(
                [](auto*... comp) {
                    return std::tuple<Components...>((*comp)...);
                },
                comps_);
        }

        constexpr auto operator++() noexcept -> iterator& {
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (++std::get<Is>(comps_), ...);
            }(std::index_sequence_for<Components...>());
            return *this;
        }

        constexpr auto operator++(int) noexcept -> iterator {
            iterator temp = *this;
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (++std::get<Is>(comps_), ...);
            }(std::index_sequence_for<Components...>());
            return temp;
        }

        constexpr std::strong_ordering
            operator<=>(const iterator& that) const noexcept {
            return std::get<0>(comps_) <=> std::get<0>(that.comps_);
        }

        constexpr bool operator==(const iterator& that) const noexcept {
            return std::get<0>(comps_) == std::get<0>(that.comps_);
        }

        constexpr bool operator!=(const iterator& that) const noexcept {
            return std::get<0>(comps_) != std::get<0>(that.comps_);
        }

        constexpr auto operator--() noexcept -> iterator& {
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (--std::get<Is>(comps_), ...);
            }(std::index_sequence_for<Components...>());
            return *this;
        }

        constexpr auto operator--(int) noexcept -> iterator {
            iterator temp = *this;
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (--std::get<Is>(comps_), ...);
            }(std::index_sequence_for<Components...>());
            return temp;
        }

        constexpr auto operator+=(ptrdiff_t diff) noexcept -> iterator& {
            [this, diff]<size_t... Is>(std::index_sequence<Is...>) {
                ((std::get<Is>(comps_) += diff), ...);
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
                ((std::get<Is>(comps_) -= diff), ...);
            }(std::index_sequence_for<Components...>());
            return *this;
        }

        constexpr auto operator-(ptrdiff_t diff) noexcept -> iterator {
            iterator temp = *this;
            temp -= diff;
            return temp;
        }

        constexpr ptrdiff_t operator-(const iterator& that) const noexcept {
            return std::get<0>(comps_) - std::get<0>(that.comps_);
        }

        constexpr iterator() noexcept = default;

    private:
        tuple_t comps_;
    };

    iterator begin() const noexcept { return iterator{ _base::bufs }; }

    iterator end() const noexcept {
        tuple_t comps = _base::bufs;
        [this, &comps]<size_t... Is>(std::index_sequence<Is...>) {
            ((std::get<Is>(comps) += this->size), ...);
        }(std::index_sequence_for<Components...>());
        return iterator{ comps };
    }
};

template <component... Components>
class eview : private _view_base<Components...> {
    using _base = _view_base<Components...>;
    using typename _base::tuple_t;

public:
    using value_type = std::tuple<entity_t, Components...>;

    eview(const slice<std::remove_cvref_t<Components>...>& slice) noexcept
        : _view_base<Components...>(slice), entities_(slice.entities()) {}

    class iterator {
        friend class eview;
        iterator(const entity_t* entity, const tuple_t& comps) noexcept
            : entity_(entity), comps_(comps) {}

    public:
        using value_type       = std::tuple<entity_t, Components...>;
        using reference_type   = value_type;
        using size_type        = size_t;
        using difference_type  = ptrdiff_t;
        using iterator_concept = std::contiguous_iterator_tag;

        constexpr auto operator*() const noexcept
            -> std::tuple<entity_t, Components...> {
            return std::apply(
                [this](auto*... comp) {
                    return std::tuple<entity_t, Components...>(
                        *entity_, (*comp)...);
                },
                comps_);
        }

        constexpr auto operator++() noexcept -> iterator& {
            ++entity_; // NOLINT
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (++std::get<Is>(comps_), ...);
            }(std::index_sequence_for<Components...>());
            return *this;
        }

        constexpr auto operator++(int) noexcept -> iterator {
            iterator temp = *this;
            ++entity_; // NOLINT
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (++std::get<Is>(comps_), ...);
            }(std::index_sequence_for<Components...>());
            return temp;
        }

        constexpr std::strong_ordering
            operator<=>(const iterator& that) const noexcept {
            return entity_ <=> that.entity_;
        }

        constexpr bool operator==(const iterator& that) const noexcept {
            return entity_ == that.entity_;
        }

        constexpr bool operator!=(const iterator& that) const noexcept {
            return entity_ != that.entity_;
        }

        constexpr auto operator--() noexcept -> iterator& {
            --entity_; // NOLINT
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (--std::get<Is>(comps_), ...);
            }(std::index_sequence_for<Components...>());
            return *this;
        }

        constexpr auto operator--(int) noexcept -> iterator {
            iterator temp = *this;
            --entity_; // NOLINT
            [this]<size_t... Is>(std::index_sequence<Is...>) {
                (--std::get<Is>(comps_), ...);
            }(std::index_sequence_for<Components...>());
            return temp;
        }

        constexpr auto operator+=(ptrdiff_t diff) noexcept -> iterator& {
            entity_ += diff; // NOLINT
            [this, diff]<size_t... Is>(std::index_sequence<Is...>) {
                ((std::get<Is>(comps_) += diff), ...);
            }(std::index_sequence_for<Components...>());
            return *this;
        }

        constexpr auto operator+(ptrdiff_t diff) const noexcept -> iterator {
            iterator temp = *this;
            temp += diff;
            return temp;
        }

        constexpr auto operator-=(ptrdiff_t diff) noexcept -> iterator& {
            entity_ -= diff; // NOLINT
            [this, diff]<size_t... Is>(std::index_sequence<Is...>) {
                ((std::get<Is>(comps_) -= diff), ...);
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

        iterator() noexcept = default;

    private:
        const entity_t* entity_{};
        tuple_t comps_;
    };

    iterator begin() const noexcept {
        return iterator{ entities_, this->bufs };
    }

    iterator end() const noexcept {
        tuple_t comps = _base::bufs;
        [this, &comps]<size_t... Is>(std::index_sequence<Is...>) {
            ((std::get<Is>(comps) += this->size), ...);
        }(std::index_sequence_for<Components...>());
        return iterator{ entities_ + this->size, comps };
    }

private:
    const entity_t* entities_;
};

template <typename... Filters>
class query {
    template <typename Alloc>
    using _manual_querior_t =
        _basic_querior::basic_querior<Alloc, false, Filters...>;

    template <typename Alloc>
    using _cached_querior_t =
        _basic_querior::basic_querior<Alloc, true, Filters...>;

public:
    using filters_type = type_list<Filters...>;
    using storage_filters_type =
        type_list<typename _query::_storage_filter<Filters>::type...>;
    using component_list = type_list_recurse_expose_t<
        bundle,
        type_list_expose_t<with, type_list_filt_t<_is_with, filters_type>>,
        same_cvref>;
    using nempty_comp_list    = type_list_filt_t<_not_empty, component_list>;
    using manual_querior_type = _manual_querior_t<std::allocator<std::byte>>;
    using cached_querior_type = _cached_querior_t<std::allocator<std::byte>>;
    using fetchable_filters   = typename _cached_querior_t<
          std::allocator<std::byte>>::fetchable_filters;
    using rnempty_comp_list =
        type_list_convert_t<std::remove_cvref, nempty_comp_list>;
    using slice_t = type_list_rebind_t<slice, rnempty_comp_list>;
    using view_t  = type_list_rebind_t<view, nempty_comp_list>;
    using eview_t = type_list_rebind_t<eview, nempty_comp_list>;

    template <typename Alloc, bool Construct, typename... QueriorFilters>
    requires(
        std::same_as<type_list<QueriorFilters...>, filters_type> ||
        std::same_as<type_list<QueriorFilters...>, storage_filters_type>)
    explicit query(
        _basic_querior::basic_querior<Alloc, Construct, QueriorFilters...>&
            querior) noexcept
        : slices_(querior.slices()) {}

    query() noexcept = default;

    ATOM_NODISCARD auto get() const noexcept
    requires std::negation_v<is_empty_template<nempty_comp_list>>
    {
        if constexpr (type_list_size_v<fetchable_filters> == 0) {
            return slices_ |
                   std::views::transform([](const slice_t& slice) noexcept {
                       return view_t{ slice };
                   }) |
                   std::views::join;
        } else {
            // TODO:
            static_assert(false);
        }
    }

    ATOM_NODISCARD auto get_with_entity() const noexcept {
        if constexpr (type_list_size_v<fetchable_filters> == 0) {
            return slices_ |
                   std::views::transform([](const slice_t& slice) noexcept {
                       return eview_t{ slice };
                   }) |
                   std::views::join;
        } else {
            // TODO:
            static_assert(false);
        }
    }

    ATOM_NODISCARD auto entities() const noexcept {
        if constexpr (type_list_size_v<fetchable_filters> == 0) {
            return slices_ |
                   std::views::transform([](const slice_t& slice) noexcept {
                       return std::span<const entity_t>{ slice.entities(),
                                                         slice.size() };
                   }) |
                   std::views::join;
        } else {
            // TODO:
            static_assert(false);
        }
    }

private:
    std::span<const slice_t> slices_{};

    template <auto Sys, typename Argument, size_t IndexOfSysInSysList>
    friend struct construct_from_world_t;
};

template <auto Sys, typename... Filters, size_t Index>
struct construct_from_world_t<Sys, query<Filters...>, Index> {
    template <typename Ty>
    using _predicate = internal::_is_relevant_query_tuple<Sys, Ty>;

    template <world World>
    query<Filters...> operator()(World& world) const {
        using query_t        = query<Filters...>;
        using cached_querior = _basic_querior::basic_querior<
            typename World::allocator_type, true, Filters...>;
        using queries_tuple = type_list_first_t<
            type_list_filt_t<_predicate, typename World::queries>>;

        auto& queries = world_accessor::queries(world);
        auto& tuple   = neutron::get_first<queries_tuple>(queries);
        auto& querior = tuple.template get_first<cached_querior>();
        querior.sync(world);
        return query_t{ querior };
    }
};

} // namespace neutron
