#pragma once
#include <functional>
#include <type_traits>
#include "neutron/type_traits.hpp"
#include "../src/neutron/internal/macros.hpp"
#include "../src/neutron/internal/spreader.hpp"

namespace neutron {

/**
 * @class delegate
 * @brief A thin wrapper of function pointer and context.
 * It prefers performance over usability. We can only bind it by template
 * non-type arugment, which means it could be inlined by compiler as much as
 * possible.
 * @tparam Ret Return type of the function.
 * @tparam Args Arguments of the function.
 */
template <typename>
class delegate;

template <typename Ret, typename... Args>
class delegate<Ret(Args...)> {
public:
    using self_type     = delegate;
    using return_type   = Ret;
    using function_type = Ret(void const*, Args...);
    using type          = Ret(Args...);

    constexpr delegate() noexcept                      = default;
    constexpr delegate(const delegate& other) noexcept = default;
    constexpr delegate(delegate&& other) noexcept      = default;
    constexpr ~delegate() noexcept                     = default;

    /**
     * @brief Construct a new delegate object for a non-member function.
     *
     * @tparam Candidate The function address or lambda expression.
     * @param spreader Spread the non-type argument by
     * `neutron::spread_arg<Candidate>`.
     */
    template <auto Candidate>
    constexpr explicit delegate(value_spreader<Candidate> spreader) noexcept {
        bind<Candidate>();
    }

    /**
     * @brief Construct a new delegate object for a member function.
     *
     * @tparam Candidate The function address or lambda expression.
     * @tparam Type Instance type.
     * @param spreader Spread the non-type argument by
     * `neutron::spread_arg<Candidate>`.
     * @param instance The object whose member function would be called.
     */
    template <auto Candidate, typename Type>
    constexpr explicit delegate(
        value_spreader<Candidate> spreader, Type& instance) noexcept {
        bind<Candidate>(instance);
    }

    /**
     * @brief Construct a new delegate object for function address and payload.
     *
     * @param function Function address.
     * @param payload The instance. It can be nullptr when the function is
     * non-member function.
     */
    constexpr explicit delegate(
        function_type* function, void const* payload) noexcept {
        bind(function, payload);
    }

    /**
     * @brief Bind a callable to this delegate.
     *
     * @tparam Candidate
     */
    template <auto Candidate>
    constexpr void bind() noexcept {
        static_assert(
            std::is_invocable_r_v<Ret, decltype(Candidate), Args...>,
            "'Candidate' should be a complete function.");

        context_  = nullptr;
        function_ = [](void const*, Args... args) -> Ret {
            return Ret(std::invoke(Candidate, std::forward<Args>(args)...));
        };
    }

    /**
     * @brief Bind a callable to this delegate.
     *
     * @tparam Candidate
     * @tparam Type
     * @param instance
     */
    template <auto Candidate, typename Type>
    constexpr void bind(Type& instance) noexcept {
        static_assert(
            std::is_invocable_r_v<Ret, decltype(Candidate), Type*, Args...>,
            "'Candidate' should be a complete function.");

        context_  = &instance;
        function_ = [](void const* payload, Args... args) -> Ret {
            Type* type_instance = static_cast<Type*>(
                // NOLINTNEXTLINE: calling should have same qualification.
                const_cast<same_constness_t<void, Type>*>(payload));
            return Ret(
                std::invoke(
                    Candidate, *type_instance, std::forward<Args>(args)...));
        };
    }

    /**
     * @brief Bind function to this delegate
     *
     * @param function Pointer to the function
     * @param payload
     */
    constexpr void bind(function_type* function, void const* payload) noexcept {
        function_ = function;
        context_  = payload;
    }

    /**
     * @brief Call function has already bind
     *
     * @param args Arguments
     * @return Ret Function's return type
     */
    constexpr Ret operator()(Args... args) const {
        return function_(context_, std::forward<Args>(args)...);
    }

    /**
     * @brief Get the status of this delegate.
     *
     * @return true This delegate has already bind.
     * @return false This delegate has not bind yet.
     */
    constexpr operator bool() const noexcept { return function_; }

    constexpr delegate& operator=(const delegate& other) noexcept = default;

    constexpr delegate& operator=(delegate&& other) noexcept {
        // copy pointer, no check
        function_ = other.function_;
        context_  = other.context_;

        return *this;
    }

    template <typename Type>
    constexpr bool operator==(delegate<Type>& other) const noexcept {
        if constexpr (std::is_same_v<Ret(Args...), Type>) {
            return function_ == other._function && context_ == other._context;
        } else {
            return false;
        }
    }

    template <typename Type>
    constexpr bool operator!=(delegate<Type>& other) const noexcept {
        return !(*this == other);
    }

    /**
     * @brief Clear bindings
     *
     */
    constexpr void reset() noexcept {
        context_  = nullptr;
        function_ = nullptr;
    }

    /**
     * @brief Get the function pointer
     *
     * @return function_type* Pointer of function
     */
    NODISCARD constexpr function_type* target() const noexcept {
        return function_;
    }

    /**
     * @brief Get the context
     *
     * @return void const* Pointer to the context
     */
    NODISCARD constexpr const void* context() const noexcept {
        return context_;
    }

private:
    const void* context_{};
    function_type* function_{};
};

template <typename Ret, typename... Args>
class delegate<Ret(Args...) noexcept> {
public:
    using self_type     = delegate;
    using return_type   = Ret;
    using function_type = Ret(void const*, Args...) noexcept;
    using type          = Ret(Args...) noexcept;

    constexpr delegate() noexcept                      = default;
    constexpr delegate(const delegate& other) noexcept = default;
    constexpr delegate(delegate&& other) noexcept      = default;
    constexpr ~delegate() noexcept                     = default;

    /**
     * @brief Construct a new delegate object for a non-member function.
     *
     * @tparam Candidate The function address or lambda expression.
     * @param spreader Spread the non-type argument by
     * `neutron::spread_arg<Candidate>`.
     */
    template <auto Candidate>
    constexpr explicit delegate(value_spreader<Candidate> spreader) noexcept {
        bind<Candidate>();
    }

    /**
     * @brief Construct a new delegate object for a member function.
     *
     * @tparam Candidate The function address or lambda expression.
     * @tparam Type Instance type.
     * @param spreader Spread the non-type argument by
     * `neutron::spread_arg<Candidate>`.
     * @param instance The object whose member function would be called.
     */
    template <auto Candidate, typename Type>
    constexpr explicit delegate(
        value_spreader<Candidate> spreader, Type& instance) noexcept {
        bind<Candidate>(instance);
    }

    /**
     * @brief Construct a new delegate object for function address and payload.
     *
     * @param function Function address.
     * @param payload The instance. It can be nullptr when the function is
     * non-member function.
     */
    constexpr explicit delegate(
        function_type* function, void const* payload) noexcept {
        bind(function, payload);
    }

    /**
     * @brief Bind a callable to this delegate.
     *
     * @tparam Candidate
     */
    template <auto Candidate>
    constexpr void bind() noexcept {
        static_assert(
            std::is_invocable_r_v<Ret, decltype(Candidate), Args...>,
            "'Candidate' should be a complete function.");

        context_  = nullptr;
        function_ = [](void const*, Args... args) noexcept -> Ret {
            return Ret(std::invoke(Candidate, std::forward<Args>(args)...));
        };
    }

    /**
     * @brief Bind a callable to this delegate.
     *
     * @tparam Candidate
     * @tparam Type
     * @param instance
     */
    template <auto Candidate, typename Type>
    constexpr void bind(Type& instance) noexcept {
        static_assert(
            std::is_invocable_r_v<Ret, decltype(Candidate), Type*, Args...>,
            "'Candidate' should be a complete function.");

        context_  = &instance;
        function_ = [](void const* payload, Args... args) noexcept -> Ret {
            Type* type_instance = static_cast<Type*>(
                // NOLINTNEXTLINE: calling should have same qualification.
                const_cast<same_constness_t<void, Type>*>(payload));
            return Ret(
                std::invoke(
                    Candidate, *type_instance, std::forward<Args>(args)...));
        };
    }

    /**
     * @brief Bind function to this delegate
     *
     * @param function Pointer to the function
     * @param payload
     */
    constexpr void bind(function_type* function, void const* payload) noexcept {
        function_ = function;
        context_  = payload;
    }

    /**
     * @brief Call function has already bind
     *
     * @param args Arguments
     * @return Ret Function's return type
     */
    constexpr Ret operator()(Args... args) const noexcept {
        return function_(context_, std::forward<Args>(args)...);
    }

    /**
     * @brief Get the status of this delegate.
     *
     * @return true This delegate has already bind.
     * @return false This delegate has not bind yet.
     */
    constexpr operator bool() const noexcept { return function_; }

    constexpr delegate& operator=(const delegate& other) noexcept = default;

    constexpr delegate& operator=(delegate&& other) noexcept = default;

    /**
     * @brief Clear bindings
     *
     */
    constexpr void reset() noexcept {
        context_  = nullptr;
        function_ = nullptr;
    }

    /**
     * @brief Get the function pointer
     *
     * @return function_type* Pointer of function
     */
    NODISCARD constexpr function_type* target() const noexcept {
        return function_;
    }

    /**
     * @brief Get the context
     *
     * @return void const* Pointer to the context
     */
    NODISCARD constexpr const void* context() const noexcept {
        return context_;
    }

private:
    const void* context_{};
    function_type* function_{};
};

} // namespace neutron

template <typename Ret, typename... Args>
constexpr bool operator==(
    const neutron::delegate<Ret(Args...)>& lhs,
    const neutron::delegate<Ret(Args...)>& rhs) noexcept {
    return lhs.target() == rhs.target() && lhs.context() == rhs.context();
}
template <typename Ret, typename... Args>
constexpr bool operator==(
    const neutron::delegate<Ret(Args...)>& lhs,
    const neutron::delegate<Ret(Args...) noexcept>& rhs) noexcept {
    return lhs.target() == rhs.target() && lhs.context() == rhs.context();
}
template <typename Ret, typename... Args>
constexpr bool operator==(
    const neutron::delegate<Ret(Args...) noexcept>& lhs,
    const neutron::delegate<Ret(Args...)>& rhs) noexcept {
    return lhs.target() == rhs.target() && lhs.context() == rhs.context();
}
template <typename Ret, typename... Args>
constexpr bool operator==(
    const neutron::delegate<Ret(Args...) noexcept>& lhs,
    const neutron::delegate<Ret(Args...) noexcept>& rhs) noexcept {
    return lhs.target() == rhs.target() && lhs.context() == rhs.context();
}
template <typename Ret, typename... Args>
constexpr bool operator!=(
    const neutron::delegate<Ret(Args...)>& lhs,
    const neutron::delegate<Ret(Args...)>& rhs) noexcept {
    return lhs.target() != rhs.target() || lhs.context() != rhs.context();
}
template <typename Ret, typename... Args>
constexpr bool operator!=(
    const neutron::delegate<Ret(Args...)>& lhs,
    const neutron::delegate<Ret(Args...) noexcept>& rhs) noexcept {
    return lhs.target() != rhs.target() || lhs.context() != rhs.context();
}
template <typename Ret, typename... Args>
constexpr bool operator!=(
    const neutron::delegate<Ret(Args...) noexcept>& lhs,
    const neutron::delegate<Ret(Args...)>& rhs) noexcept {
    return lhs.target() != rhs.target() || lhs.context() != rhs.context();
}
template <typename Ret, typename... Args>
constexpr bool operator!=(
    const neutron::delegate<Ret(Args...) noexcept>& lhs,
    const neutron::delegate<Ret(Args...) noexcept>& rhs) noexcept {
    return lhs.target() != rhs.target() || lhs.context() != rhs.context();
}
