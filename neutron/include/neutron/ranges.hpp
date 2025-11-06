#pragma once
#include <ranges>
#include "neutron/neutron.hpp"
#include "pair.hpp"

#if __cplusplus > 202002L
template <typename Derived>
using range_adaptor_closure = std::ranges::range_adaptor_closure<Derived>;
#else

namespace neutron {

template <typename Derived>
struct range_adaptor_closure {
    friend Derived;

private:
    range_adaptor_closure() noexcept = default;
};

template <typename Ty>
concept _range_adaptor_closure_object =
    std::is_base_of_v<range_adaptor_closure<std::remove_cvref_t<Ty>>, std::remove_cvref_t<Ty>>;

/**
 * @brief The custom result of pipeline operator between two closures. It's a special closure.
 *
 * @tparam First The type of the first range closure.
 * @tparam Second The type of the second range closure.
 */
template <typename First, typename Second>
struct _range_pipeline_result
    : public range_adaptor_closure<_range_pipeline_result<First, Second>> {

    template <typename Left, typename Right>
    constexpr _range_pipeline_result(Left&& left, Right&& right) noexcept(
        std::is_nothrow_constructible_v<compressed_pair<First, Second>, Left, Right>)
        : closures_(std::forward<Left>(left), std::forward<Right>(right)) {}

    constexpr ~_range_pipeline_result() noexcept(
        std::is_nothrow_destructible_v<compressed_pair<First, Second>>) = default;

    constexpr _range_pipeline_result(const _range_pipeline_result&) noexcept(
        std::is_nothrow_copy_constructible_v<compressed_pair<First, Second>>) = default;

    constexpr _range_pipeline_result(_range_pipeline_result&& that) noexcept(
        std::is_nothrow_move_constructible_v<compressed_pair<First, Second>>)
        : closures_(std::move(that.closures_)) {}

    constexpr _range_pipeline_result& operator=(const _range_pipeline_result&) noexcept(
        std::is_nothrow_copy_assignable_v<compressed_pair<First, Second>>) = default;

    constexpr _range_pipeline_result& operator=(_range_pipeline_result&& that) noexcept(
        std::is_nothrow_move_assignable_v<compressed_pair<First, Second>>) {
        closures_ = std::move(that.closures_);
    }

    /**
     * @brief Get out of the pipeline.
     *
     * Usually, the tparam will be a range, but not only.
     * It depends on the closure.
     * @tparam Arg This tparam could be deduced.
     */
    template <typename Arg>
    requires requires {
        std::forward<First>(std::declval<First>())(std::declval<Arg>());
        std::forward<Second>(std::declval<Second>())(
            std::forward<First>(std::declval<First>())(std::declval<Arg>()));
    }
    [[nodiscard]] constexpr auto operator()(Arg&& arg) noexcept(noexcept(std::forward<Second>(
        closures_.second())(std::forward<First>(closures_.first())(std::forward<Arg>(arg))))) {
        return std::forward<Second>(closures_.second())(
            std::forward<First>(closures_.first())(std::forward<Arg>(arg)));
    }

    /**
     * @brief Get out of the pipeline.
     *
     * Usually, the tparam will be a range, but not only.
     * It depends on the closure.
     * @tparam Arg This tparam could be deduced.
     */
    template <typename Arg>
    requires requires {
        std::forward<First>(std::declval<First>())(std::declval<Arg>());
        std::forward<Second>(std::declval<Second>())(
            std::forward<First>(std::declval<First>())(std::declval<Arg>()));
    }
    [[nodiscard]] constexpr auto operator()(Arg&& arg) const noexcept(noexcept(std::forward<Second>(
        closures_.second())(std::forward<First>(closures_.first())(std::forward<Arg>(arg))))) {
        return std::forward<Second>(closures_.second())(
            std::forward<First>(closures_.first())(std::forward<Arg>(arg)));
    }

private:
    compressed_pair<First, Second> closures_;
};

/**
 * @brief Closure.
 *
 * It could be used as range adaptor closure.
 * @tparam Fn Function called in operator()
 * @tparam Args Arguments that
 */
template <typename Fn, typename... Args>
class closure : range_adaptor_closure<closure<Fn, Args...>> {
    using index_sequence = std::index_sequence_for<Args...>;
    using self_type      = closure;

public:
    using pipeline_tag = void;

    static_assert((std::same_as<std::decay_t<Args>, Args> && ...));
    static_assert(std::is_empty_v<Fn> || std::is_default_constructible_v<Fn>);

    explicit constexpr closure(auto&&... args) noexcept(
        std::conjunction_v<std::is_nothrow_constructible<std::tuple<Args...>, decltype(args)...>>)
    requires std::is_constructible_v<std::tuple<Args...>, decltype(args)...>
        : args_(std::make_tuple(std::forward<decltype(args)>(args)...)) {}

    constexpr decltype(auto) operator()(auto&& arg) & noexcept(
        noexcept(_call(*this, std::forward<decltype(arg)>(arg), index_sequence{})))
    requires std::invocable<Fn, decltype(arg), Args&...>
    {
        return _call(*this, std::forward<decltype(arg)>(arg), index_sequence{});
    }

    constexpr decltype(auto) operator()(auto&& arg) const& noexcept(
        noexcept(_call(*this, std::forward<decltype(arg)>(arg), index_sequence{})))
    requires std::invocable<Fn, decltype(arg), const Args&...>
    {
        return _call(*this, std::forward<decltype(arg)>(arg), index_sequence{});
    }

    template <typename Ty>
    requires std::invocable<Fn, Ty, Args...>
    constexpr decltype(auto) operator()(Ty&& arg) && noexcept(
        noexcept(_call(*this, std::forward<Ty>(arg), index_sequence{}))) {
        return _call(*this, std::forward<Ty>(arg), index_sequence{});
    }

    template <typename Ty>
    requires std::invocable<Fn, Ty, const Args...>
    constexpr decltype(auto) operator()(Ty&& arg) const&& noexcept(
        noexcept(_call(*this, std::forward<Ty>(arg), index_sequence{}))) {
        return _call(*this, std::forward<Ty>(arg), index_sequence{});
    }

private:
    template <typename SelfTy, typename Ty, size_t... Is>
    constexpr static decltype(auto)
        _call(SelfTy&& self, Ty&& arg, std::index_sequence<Is...>) noexcept(noexcept(
            Fn{}(std::forward<Ty>(arg), std::get<Is>(std::forward<SelfTy>(self).args_)...))) {
        static_assert(std::same_as<std::index_sequence<Is...>, index_sequence>);
        return Fn{}(std::forward<Ty>(arg), std::get<Is>(std::forward<SelfTy>(self).args_)...);
    }

    std::tuple<Args...> args_;
};

/**
 * @brief Making a closure without caring about the parameter types.
 *
 * @tparam Fn Closure function type.
 * @tparam Args Arguments types would be called in the closure. They could be deduced.
 * @param args Arguments.
 * @return closure<Fn, std::decay_t<Args>...> Closure.
 */
template <typename Fn, typename... Args>
constexpr auto make_closure(Args&&... args) -> closure<Fn, std::decay_t<Args>...> {
    return closure<Fn, std::decay_t<Args>...>(std::forward<Args>(args)...);
}

} // namespace neutron

template <std::ranges::range Rng, typename Closure>
requires neutron::_range_adaptor_closure_object<Closure>
constexpr decltype(auto) operator|(Rng&& range, Closure&& closure) noexcept(requires {
    std::forward<Closure>(closure)(std::forward<Rng>(range));
}) {
    return std::forward<Closure>(closure)(std::forward<Rng>(range));
}

template <typename Closure1, typename Closure2>
requires neutron::_range_adaptor_closure_object<Closure1> &&
         neutron::_range_adaptor_closure_object<Closure2>
constexpr decltype(auto) operator|(Closure1&& closure1, Closure2&& closure2) {
    return neutron::_range_pipeline_result<Closure1, Closure2>(
        std::forward<Closure1>(closure1), std::forward<Closure2>(closure2));
}

template <typename Rng>
struct phony_iterator {
public:
    phony_iterator& operator++() noexcept { return *this; }
    phony_iterator operator++(int) noexcept { return *this; }
    phony_iterator& operator+=(int) noexcept { return *this; }
    phony_iterator& operator+(int) noexcept { return *this; }
    phony_iterator& operator--() noexcept { return *this; }
    phony_iterator operator--(int) noexcept { return *this; }
    phony_iterator& operator-=(int) noexcept { return *this; }
    phony_iterator& operator-(int) noexcept { return *this; }

    phony_iterator& operator*() noexcept { return *this; }
    const phony_iterator& operator*() const noexcept { return *this; }
    Rng* operator->() noexcept { return nullptr; }
    const Rng* operator->() const noexcept { return nullptr; }
};

class phony_range {
public:
    using iterator       = phony_iterator<phony_range>;
    using const_iterator = phony_iterator<phony_range>;

    // NOLINTBEGIN(readability-convert-member-functions-to-static)

    NODISCARD constexpr iterator begin() noexcept { return {}; }
    NODISCARD constexpr iterator begin() const noexcept { return {}; }
    NODISCARD constexpr iterator end() noexcept { return {}; }
    NODISCARD constexpr iterator end() const noexcept { return {}; }

    // NOLINTEND(readability-convert-member-functions-to-static)
};

template <typename WildClosure, typename Closure>
requires neutron::_range_adaptor_closure_object<Closure>
constexpr decltype(auto) operator|(WildClosure&& wild, Closure&& closure) {
    return neutron::_range_pipeline_result<WildClosure, Closure>(
        std::forward<WildClosure>(wild), std::forward<Closure>(closure));
}

template <typename Closure, typename WildClosure>
requires neutron::_range_adaptor_closure_object<Closure>
constexpr decltype(auto) operator|(Closure&& closure, WildClosure&& wild) {
    return neutron::_range_pipeline_result<Closure, WildClosure>(
        std::forward<Closure>(closure), std::forward<WildClosure>(wild));
}

#endif

namespace neutron {

namespace concepts {
template <typename Iter>
concept input_iterator = requires(const Iter iter) { *iter; };

template <typename Iter>
concept output_iterator = requires(Iter iter) { *iter; };

template <typename Iter>
concept forward_iterator = input_iterator<Iter> && requires(Iter iter) {
    ++iter;
    iter++;
};

template <typename Iter>
concept bidirectional_iterator = forward_iterator<Iter> && requires(Iter iter) {
    --iter;
    iter--;
};

template <typename Iter>
concept random_access_iterator =
    bidirectional_iterator<Iter> && requires(Iter iter) { iter += static_cast<unsigned>(-1); };

// concept: ref_convertible
template <typename Rng, typename Container>
concept ref_convertible =
    !std::ranges::input_range<Container> ||
    std::convertible_to<std::ranges::range_reference_t<Rng>, std::ranges::range_value_t<Container>>;

// concept: common_range
/**
 * @brief A range whose iterator type is the same as the sentinel type.
 *
 */
template <typename Rng>
concept common_range = ::std::ranges::range<Rng> &&
                       ::std::same_as<std::ranges::iterator_t<Rng>, ::std::ranges::sentinel_t<Rng>>;

// concept: common_constructible
/**
 * @brief Containers that could be constructed form the given range and arguments.
 *
 * Range should has input iterator, and we could construct the contains via this.
 */
template <typename Rng, typename Container, typename... Args>
concept common_constructible =
    neutron::concepts::common_range<Rng> &&
    // using of iterator_category
    requires { typename std::iterator_traits<std::ranges::iterator_t<Rng>>::iterator_category; } &&
    // input_range_tag or derived from input_range_tag
    (std::same_as<
         typename std::iterator_traits<std::ranges::iterator_t<Rng>>::iterator_category,
         std::input_iterator_tag> ||
     std::derived_from<
         typename std::iterator_traits<std::ranges::iterator_t<Rng>>::iterator_category,
         std::input_iterator_tag>) &&
    // constructible from iterator (and arguments)
    std::constructible_from<
        Container, std::ranges::iterator_t<Rng>, std::ranges::iterator_t<Rng>, Args...>;

// concept: can_emplace_back
template <typename Container, typename Reference>
concept can_emplace_back =
    requires(Container& cnt) { cnt.emplace_back(std::declval<Reference>()); };

// concept: can_push_back
template <typename Container, typename Reference>
concept can_push_back = requires(Container& cnt) { cnt.push_back(std::declval<Reference>()); };

// concept: can_emplace_end
template <typename Container, typename Reference>
concept can_emplace_end =
    requires(Container& cnt) { cnt.emplace(cnt.end(), std::declval<Reference>()); };

// concept: can_insert_end
template <typename Container, typename Reference>
concept can_insert_end =
    requires(Container& cnt) { cnt.insert(cnt.end(), std::declval<Reference>()); };

// concept: constructible_appendable
/**
 * @brief Contains could be constructed from arguments and then append elements to it from range.
 *
 */
template <typename Rng, typename Container, typename... Args>
concept constructible_appendable =
    std::constructible_from<Container, Args...> &&
    (can_emplace_back<Container, std::ranges::range_reference_t<Rng>> ||
     can_push_back<Container, std::ranges::range_reference_t<Rng>> ||
     can_emplace_end<Container, std::ranges::range_reference_t<Rng>> ||
     can_insert_end<Container, std::ranges::range_reference_t<Rng>>);

template <typename Iter, typename Ty>
concept constructible_from_iterator =
    std::same_as<typename std::iterator_traits<Iter>::value_type, Ty>;

template <typename Rng, typename Ty>
concept compatible_range =
    std::ranges::range<Rng> && std::same_as<std::ranges::range_value_t<Rng>, Ty>;

template <typename Rng>
concept map_like = std::ranges::range<Rng> && (requires {
                       typename Rng::key_type;
                       typename Rng::mapped_type;
                   } || (_pair<std::ranges::range_value_t<Rng>> &&
                         std::is_const_v<typename std::ranges::range_value_t<Rng>::first_type>));

} // namespace concepts

namespace ranges {
/**
 * @brief Phony input iterator, for lazy construction with ranges.
 *
 * Can not really iterate.
 * @tparam Rng
 */
template <std::ranges::input_range Rng>
struct phony_input_iterator {
    using iterator_category = std::input_iterator_tag;
    using value_type        = std::ranges::range_value_t<Rng>;
    using difference_type   = ptrdiff_t;
    using pointer           = std::add_pointer_t<std::ranges::range_reference_t<Rng>>;
    using reference         = std::ranges::range_reference_t<Rng>;

    reference operator*() const = delete;
    pointer operator->() const  = delete;

    phony_input_iterator& operator++()   = delete;
    phony_input_iterator operator++(int) = delete;

    bool operator==(const phony_input_iterator&) const = delete;
    bool operator!=(const phony_input_iterator&) const = delete;
};

// NOLINTBEGIN(cppcoreguidelines-require-return-statement)

/**
 * @brief Construct a container from a range and arguments.
 *
 */
template <typename Container, std::ranges::input_range Rng, typename... Args>
requires(!std::ranges::view<Container>)
[[nodiscard]] constexpr Container to(Rng&& range, Args&&... args) {
#if HAS_CXX23
    return std::ranges::to<Container>(std::forward<Rng>(range), std::forward<Args>(args)...);
#else
    static_assert(!std::is_const_v<Container>, "Container must not be const.");
    static_assert(!std::is_volatile_v<Container>, "Container must not be volatile.");
    static_assert(std::is_class_v<Container>, "Container must be a class type.");

    if constexpr (concepts::ref_convertible<Rng, Container>) {
        if constexpr (std::constructible_from<Container, Rng, Args...>) {
            return Container(std::forward<Rng>(range), std::forward<Args>(args)...);
        }
        // there is no std::from_range_t in cpp20
        /* else if constexpr (std::constructible_from<
                               Container,
                               const std::from_range_t&,
                               Rng,
                               Args...>) {
            return Container(
                std::from_range, std::forward<Rng>(range), std::forward<Args>(args)...
            );
        } */
        else if constexpr (concepts::common_constructible<Rng, Container, Args...>) {
            return Container(
                std::ranges::begin(range), std::ranges::end(range), std::forward<Args>(args)...);
        } else if constexpr (concepts::constructible_appendable<Rng, Container, Args...>) {
            Container cnt(std::forward<Args>(args)...);
            if constexpr (
                std::ranges::sized_range<Rng> && std::ranges::sized_range<Container> &&
                requires(Container& cnt, const std::ranges::range_size_t<Container> count) {
                    cnt.reserve(count);
                    { cnt.capacity() } -> std::same_as<std::ranges::range_size_t<Container>>;
                    { cnt.max_size() } -> std::same_as<std::ranges::range_size_t<Container>>;
                }) {
                cnt.reserve(
                    static_cast<std::ranges::range_size_t<Container>>(std::ranges::size(range)));
            }
            for (auto&& elem : range) {
                using element_type = decltype(elem);
                if constexpr (concepts::can_emplace_back<Container, element_type>) {
                    cnt.emplace_back(std::forward<element_type>(elem));
                } else if constexpr (concepts::can_push_back<Container, element_type>) {
                    cnt.push_back(std::forward<element_type>(elem));
                } else if constexpr (concepts::can_emplace_end<Container, element_type>) {
                    cnt.emplace(cnt.end(), std::forward<element_type>(elem));
                } else {
                    cnt.insert(cnt.end(), std::forward<element_type>(elem));
                }
            }
            return cnt;
        } else {
            static_assert(
                false,
                "ranges::to requires the result to be constructible from the source range, "
                "either by using a suitable constructor, or by inserting each element of the range "
                "into the default-constructed object.");
        }
    } else if constexpr (std::ranges::input_range<std::ranges::range_reference_t<Rng>>) {
        const auto form = []<typename Ty>(Ty&& elem) {
            return to<std::ranges::range_value_t<Container>>(std::forward<decltype(elem)>(elem));
        };
        return to<Container>(
            std::views::transform(std::ranges::ref_view{ range }, form),
            std::forward<Args>(args)...);
    } else {
        static_assert(
            false,
            "ranges::to requires the elements of the source range to be either implicitly "
            "convertible to the elements of the destination container, or be ranges themselves "
            "for ranges::to to be applied recursively.");
    }
#endif // HAS_CXX23
}

// NOLINTEND(cppcoreguidelines-require-return-statement)

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template <typename Container>
struct to_class_fn {
    template <std::ranges::input_range Rng, typename... Args>
    [[nodiscard]] constexpr auto operator()(Rng&& range, Args&&... args)
    requires requires { to<Container>(std::forward<Rng>(range), std::forward<Args>(args)...); }
    {
        return to<Container>(std::forward<Rng>(range), std::forward<Args>(args)...);
    }
};

} // namespace internal
/*! @endcond */

/**
 * @brief Construct a container from a range.
 *
 */
template <typename Container, typename... Args>
[[nodiscard]] constexpr auto to(Args&&... args) {
#if HAS_CXX23
    return std::ranges::to<Container>(std::forward<Args>(args)...);
#else
    return make_closure<internal::to_class_fn<Container>>(std::forward<Args>(args)...);
#endif
}

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

// just for decltype
template <template <typename...> typename Cnt, typename Rng, typename... Args>
auto to_helper() {
    if constexpr (requires { Cnt(std::declval<Rng>(), std::declval<Args>()...); }) {
        return static_cast<decltype(Cnt(std::declval<Rng>(), std::declval<Args>()...))*>(nullptr);
    }
    // there is no std::from_range in cpp20
    /* else if constexpr (requires {
                           Cnt(std::from_range, std::declval<Rng>(), declval<Args>()...);
                       }) {
        return static_cast<
            decltype(Cnt(std::from_range, std::declval<Rng>(), std::declval<Args>()...))*>(nullptr);
    } */
    else if constexpr (requires {
                           Cnt(std::declval<phony_input_iterator<Rng>>(),
                               std::declval<phony_input_iterator<Rng>>(), std::declval<Args>()...);
                       }) {
        return static_cast<decltype(Cnt(
            std::declval<phony_input_iterator<Rng>>(), std::declval<phony_input_iterator<Rng>>(),
            std::declval<Args>()...))*>(nullptr);
    } else {
        static_assert(
            false,
            "No suitable way to deduce the type of the container, please provide more information "
            "about the container to use other 'to' function template.");
    }
}

} // namespace internal
/*! @endcond */

/**
 * @brief Construct a container from a range.
 *
 */

/**
 * @brief Construct a container from a range.
 *
 * @tparam Container The container need to construct.
 * @tparam Rng The range.
 * @tparam Args Other arguments.
 * @param range
 * @param args
 * @return Deduced In most times, it could be deduced to as a completed type.
 */
template <
    template <typename...> typename Container, std::ranges::input_range Rng, typename... Args,
    typename Deduced = std::remove_pointer_t<
        decltype(neutron::ranges::internal::to_helper<Container, Rng, Args...>())>>
[[nodiscard]] constexpr auto to(Rng&& range, Args&&... args) -> Deduced {
#if HAS_CXX23
    return std::ranges::to<Container>(std::forward<Rng>(range), std::forward<Args>(args)...);
#else
    return to<Deduced>(std::forward<Rng>(range), std::forward<Args>(args)...);
#endif
}

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template <template <typename...> typename Container>
struct to_template_fn {
    template <
        std::ranges::input_range Rng, typename... Args,
        typename Deduced =
            std::remove_pointer_t<decltype(internal::to_helper<Container, Rng, Args...>())>>
    [[nodiscard]] constexpr auto operator()(Rng&& range, Args&&... args) {
        return to<Deduced>(std::forward<Rng>(range), std::forward<Args>(args)...);
    }
};

} // namespace internal
/*! @endcond */

// compiler and analyzer always deduce we are using another override.
// so, changed the function name
template <template <typename...> typename Container, typename... Args>
[[nodiscard]] constexpr auto to(Args&&... args) {
#if HAS_CXX23
    return std::ranges::to<Container>(std::forward<Args>(args)...);
#else
    return make_closure<internal::to_template_fn<Container>>(std::forward<Args>(args)...);
#endif
}

namespace views {}

} // namespace ranges

namespace internal {
/**
 * @brief Fake copy init, just for compile-time deduce & check.
 * @note No difination.
 */
template <typename Ty>
[[nodiscard]] Ty fake_copy_init(Ty) noexcept;
} // namespace internal

namespace concepts {

template <std::size_t Index, typename Ty>
concept std_gettible = requires(Ty& tup) {
    { std::get<Index>(tup) } -> std::same_as<std::tuple_element_t<Index, Ty>&>;
};

template <std::size_t Index, typename Ty>
concept member_gettible = requires(Ty& tup) {
    { tup.template get<Index>() } -> std::same_as<std::tuple_element_t<Index, Ty>&>;
};

template <std::size_t Index, typename Ty>
concept adl_gettible = requires(Ty& tup) {
    { internal::fake_copy_init(get<Index>(tup)) } -> std::same_as<std::tuple_element_t<Index, Ty>>;
};

template <std::size_t Index, typename Ty>
concept gettible = std_gettible<Index, Ty> || member_gettible<Index, Ty> || adl_gettible<Index, Ty>;

} // namespace concepts

namespace ranges {

template <std::size_t Index, typename Ty>
requires concepts::gettible<Index, Ty>
constexpr decltype(auto) _get(Ty& inst) noexcept {
    using namespace concepts;
    if constexpr (std_gettible<Index, Ty>) {
        return std::get<Index>(inst);
    } else if constexpr (member_gettible<Index, Ty>) {
        return inst.template get<Index>();
    } else if constexpr (adl_gettible<Index, Ty>) {
        return get<Index>(inst);
    }
}

namespace views {

template <std::ranges::range Rng, size_t Index, bool IsConst>
struct element_iterator {
    using iterator_type   = std::ranges::iterator_t<Rng>;
    using value_type      = std::tuple_element_t<Index, std::ranges::range_value_t<Rng>>;
    using difference_type = ptrdiff_t;

    constexpr element_iterator() = default;

    constexpr element_iterator(std::ranges::iterator_t<Rng> iter) noexcept(
        std::is_nothrow_move_constructible_v<std::ranges::iterator_t<Rng>>)
        : iter_(std::move(iter)) {}

    [[nodiscard]] constexpr decltype(auto) operator*() noexcept
    requires(!IsConst)
    {
        if constexpr (concepts::gettible<
                          Index, std::remove_const_t<std::ranges::range_value_t<Rng>>>) {
            return _get<Index>(*iter_);
        } else {
            static_assert(false, "No suitable method to get the value.");
        }
    }

    [[nodiscard]] constexpr decltype(auto) operator*() const noexcept {
        if constexpr (concepts::gettible<
                          Index, std::remove_const_t<std::ranges::range_value_t<Rng>>>) {
            return _get<Index>(*iter_);
        } else {
            static_assert(false, "No suitable method to get the value.");
        }
    }

    [[nodiscard]] constexpr decltype(auto) operator->() noexcept
    requires(!IsConst)
    {
        return iter_;
    }

    [[nodiscard]] constexpr decltype(auto) operator->() const noexcept { return iter_; }

    constexpr element_iterator& operator++() noexcept(noexcept(++iter_)) {
        ++iter_;
        return *this;
    }

    constexpr element_iterator operator++(int) noexcept(
        noexcept(++iter_) && std::is_nothrow_copy_constructible_v<std::ranges::iterator_t<Rng>>) {
        auto temp = *this;
        ++iter_;
        return temp;
    }

    constexpr element_iterator& operator--() noexcept(noexcept(--iter_)) {
        --iter_;
        return *this;
    }

    constexpr element_iterator operator--(int) noexcept(
        noexcept(--iter_) && std::is_nothrow_copy_constructible_v<std::ranges::iterator_t<Rng>>) {
        auto temp = *this;
        --iter_;
        return temp;
    }

    constexpr element_iterator&
        operator+=(const difference_type offset) noexcept(noexcept(iter_ += offset))
    requires std::ranges::random_access_range<Rng>
    {
        iter_ += offset;
        return *this;
    }

    constexpr element_iterator operator+(const difference_type offset)
    requires std::ranges::random_access_range<Rng>
    {
        auto temp = *this;
        temp += offset;
        return temp;
    }

    constexpr element_iterator&
        operator-=(const difference_type offset) noexcept(noexcept(iter_ -= offset))
    requires std::ranges::random_access_range<Rng>
    {
        iter_ -= offset;
        return *this;
    }

    constexpr element_iterator operator-(const difference_type offset)
    requires std::ranges::random_access_range<Rng>
    {
        auto temp = *this;
        temp -= offset;
        return temp;
    }

    template <bool Const>
    constexpr bool operator==(const element_iterator<Rng, Index, Const>& that) const noexcept
    requires requires(iterator_type& lhs, iterator_type& rhs) { lhs == rhs; }
    {
        return iter_ == that.iter_;
    }

    template <bool Const>
    constexpr bool operator!=(const element_iterator<Rng, Index, Const>& that) const noexcept
    requires requires(iterator_type& lhs, iterator_type& rhs) { lhs != rhs; }
    {
        return iter_ != that.iter_;
    }

private:
    std::ranges::iterator_t<Rng> iter_;
};

/**
 * @brief Elements view, but supports user defined get.
 *
 * @tparam Vw
 * @tparam Index
 */
template <std::ranges::input_range Vw, size_t Index>
class elements_view : std::ranges::view_interface<elements_view<Vw, Index>> {
public:
    /**
     * @brief Constuct a elements view.
     *
     * @warning Move then constructing.
     */
    constexpr explicit elements_view(Vw range) noexcept(std::is_nothrow_constructible_v<Vw>)
        : range_(std::move(range)) {}

    constexpr auto begin() noexcept {
        return element_iterator<Vw, Index, false>(std::ranges::begin(range_));
    }

    constexpr auto begin() const noexcept {
        if constexpr (requires { std::ranges::cbegin(range_); }) {
            return element_iterator<Vw, Index, true>(std::ranges::cbegin(range_));
        } else {
            return element_iterator<Vw, Index, true>(std::ranges::begin(range_));
        }
    }

#if !HAS_CXX23
    constexpr auto cbegin() const noexcept {
        return element_iterator<Vw, Index, true>(std::ranges::cbegin(range_));
    }

    constexpr auto rbegin() noexcept {
        return element_iterator<Vw, Index, false>(std::ranges::rbegin(range_));
    }

    constexpr auto crbegin() const noexcept {
        return element_iterator<Vw, Index, true>(std::ranges::crbegin(range_));
    }
#endif

    constexpr auto end() noexcept {
        return element_iterator<Vw, Index, false>(std::ranges::end(range_));
    }

    constexpr auto end() const noexcept {
        if constexpr (requires { std::ranges::cend(range_); }) {
            return element_iterator<Vw, Index, true>(std::ranges::cend(range_));
        } else if constexpr (requires { std::end(range_); }) {
            return element_iterator<Vw, Index, true>(std::ranges::end(range_));
        } else {
            static_assert(false, "No suitable method to get a const iterator");
        }
    }

    constexpr auto cend() const noexcept { return std::ranges::cend(range_); }

    constexpr auto rend() noexcept { return std::ranges::rend(range_); }

    constexpr auto size() const noexcept
    requires std::ranges::sized_range<const Vw>
    {
        return std::ranges::size(range_);
    }

private:
    Vw range_;
};

template <size_t Index>
struct element_fn : range_adaptor_closure<element_fn<Index>> {

    template <std::ranges::viewable_range Rng>
    requires concepts::gettible<Index, std::ranges::range_value_t<Rng>>
    [[nodiscard]] constexpr auto operator()(Rng&& range) const {
        static_assert(
            Index < std::tuple_size_v<std::ranges::range_value_t<Rng>>,
            "Index should be in [0, size)");
        return elements_view<std::views::all_t<Rng>, Index>{ std::forward<Rng>(range) };
    }
};

template <size_t Index>
constexpr element_fn<Index> elements;

constexpr inline auto keys   = elements<0>;
constexpr inline auto values = elements<1>;

} // namespace views

} // namespace ranges

namespace views = ranges::views;

} // namespace neutron
