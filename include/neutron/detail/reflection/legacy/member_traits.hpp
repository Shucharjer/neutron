// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include <string_view>

namespace neutron {

/**
 * @brief Base of class template field_traits.
 *
 */
struct basic_field_traits;

/**
 * @brief Traits of filed.
 */
template <typename = void>
struct field_traits;

struct basic_field_traits {
    explicit constexpr basic_field_traits(const char* name) noexcept
        : name_(name) {}
    explicit constexpr basic_field_traits(std::string_view name) noexcept
        : name_(name) {}

    constexpr basic_field_traits(const basic_field_traits&) noexcept = default;
    constexpr basic_field_traits&
        operator=(const basic_field_traits&) noexcept = default;

    constexpr basic_field_traits(basic_field_traits&& obj) noexcept
        : name_(obj.name_) {}
    constexpr basic_field_traits& operator=(basic_field_traits&& obj) noexcept {
        if (this != &obj) {
            name_ = obj.name_;
        }
        return *this;
    }

    constexpr virtual ~basic_field_traits() noexcept = default;

    [[nodiscard]] constexpr auto name() const noexcept -> std::string_view {
        return name_;
    }

private:
    std::string_view name_;
};

template <>
struct field_traits<void> : public basic_field_traits {
    using type = void;

    explicit constexpr field_traits() noexcept : basic_field_traits("void") {}
};

template <typename Ty>
requires(!std::is_void_v<Ty>)
struct field_traits<Ty*> : public basic_field_traits {
    using type = Ty;

    explicit constexpr field_traits(const char* name, Ty* pointer) noexcept
        : basic_field_traits(name), pointer_(pointer) {}

    [[nodiscard]] constexpr auto get() noexcept -> Ty& { return *pointer_; }
    [[nodiscard]] constexpr auto get() const noexcept -> const Ty& {
        return *pointer_;
    }

    [[nodiscard]] constexpr auto pointer() const noexcept -> Ty* {
        return pointer_;
    }

private:
    Ty* pointer_;
};

template <typename Ty, typename Class>
struct field_traits<Ty Class::*> : public basic_field_traits {
    using type = Ty;

    explicit constexpr field_traits(const char* name, Ty Class::* pointer)
        : basic_field_traits(name), pointer_(pointer) {}

    explicit constexpr field_traits(std::string_view name, Ty Class::* pointer)
        : basic_field_traits(name), pointer_(pointer) {}

    [[nodiscard]] constexpr auto get(Class& instance) const noexcept -> Ty& {
        return instance.*pointer_;
    }
    [[nodiscard]] constexpr auto get(const Class& instance) const noexcept
        -> const Ty& {
        return instance.*pointer_;
    }

    [[nodiscard]] constexpr auto pointer() const noexcept -> Ty Class::* {
        return pointer_;
    }

private:
    Ty Class::* pointer_;
};

/**
 * @brief Base of class template function_traits.
 *
 */
struct basic_function_traits;

/**
 * @brief Traits of function.
 */
template <typename>
struct function_traits;

struct basic_function_traits {
    explicit constexpr basic_function_traits(const char* name) noexcept
        : name_(name) {}
    explicit constexpr basic_function_traits(std::string_view name) noexcept
        : name_(name) {}

    constexpr basic_function_traits(const basic_function_traits&) noexcept =
        default;
    constexpr basic_function_traits&
        operator=(const basic_function_traits&) noexcept = default;

    constexpr basic_function_traits(basic_function_traits&& obj) noexcept
        : name_(obj.name_) {}
    constexpr basic_function_traits&
        operator=(basic_function_traits&& obj) noexcept {
        if (this != &obj) {
            name_ = obj.name_;
        }
        return *this;
    }

    constexpr virtual ~basic_function_traits() noexcept = default;

    [[nodiscard]] constexpr auto name() const noexcept -> std::string_view {
        return name_;
    }

private:
    std::string_view name_;
};

template <typename Ret, typename... Args>
struct function_traits<Ret (*)(Args...)> : public basic_function_traits {
    using func_type = Ret(Args...);

    explicit constexpr function_traits(
        const char* name, Ret (*pointer)(Args...)) noexcept
        : basic_function_traits(name), pointer_(pointer) {}

    [[nodiscard]] constexpr std::size_t num_args() const noexcept {
        return sizeof...(Args);
    }

    [[nodiscard]] constexpr auto call(Args&&... args) const -> Ret {
        return pointer_(std::forward<Args>(args)...);
    }

    [[nodiscard]] constexpr auto pointer() const noexcept -> Ret (*)(Args...) {
        return pointer_;
    }

private:
    Ret (*pointer_)(Args...);
};

template <typename Ret, typename Class, typename... Args>
struct function_traits<Ret (Class::*)(Args...)> : public basic_function_traits {
    using func_type = Ret(Args...);

    explicit constexpr function_traits(
        const char* name, Ret (Class::*pointer)(Args...)) noexcept
        : basic_function_traits(name), pointer_(pointer) {}

    [[nodiscard]] constexpr std::size_t num_args() const noexcept {
        return sizeof...(Args);
    }

    [[nodiscard]] constexpr auto call(Class& instance, Args&&... args) const
        -> Ret {
        (instance.*pointer_)(std::forward<Args>(args)...);
    }

    [[nodiscard]] constexpr auto pointer() const noexcept
        -> Ret (Class::*)(Args...) {
        return pointer_;
    }

private:
    Ret (Class::*pointer_)(Args...);
};


} // namespace neutron
