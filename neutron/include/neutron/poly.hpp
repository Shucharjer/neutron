#pragma once
#include <cstdlib>
#include <new>
#include <utility>
#include "neutron/internal/member_function_traits.hpp"
#include "neutron/template_list.hpp"
#include "neutron/type_hash.hpp"

namespace neutron {

struct poly_op_default_construct {
    void (*value)(void* ptr) = nullptr;
};

struct poly_op_copy_construct {
    void (*value)(void* ptr, const void* other) = nullptr;
};

struct poly_op_move_construct {
    void (*value)(void* ptr, void* other) = nullptr;
};

struct poly_op_copy_assign {
    void (*value)(void* ptr, const void* other) = nullptr;
};

struct poly_op_move_assign {
    void (*value)(void* ptr, void* other) = nullptr;
};

constexpr size_t k_default_poly_storage_size = 16;

constexpr size_t k_default_poly_storage_align = 16;

namespace internal {
struct poly_empty_impl {
    struct _universal {
        template <typename Ty>
        [[noreturn]] constexpr operator Ty&&() {
            // If you meet error here, you must called function in static vtable
            // without initializing it by an polymorphic object.
            std::abort();
        }
    };

    template <size_t Index, typename Object = void, typename... Args>
    constexpr auto invoke(Args&&...) noexcept {
        return _universal{};
    }
    template <size_t Index, typename Object = void, typename... Args>
    constexpr auto invoke(Args&&...) const noexcept {
        return _universal{};
    }
};
} // namespace internal

template <typename Ty>
concept _poly_basic_object = requires {
    // Ty should declared a class template called interface.
    typename Ty::template interface<internal::poly_empty_impl>;
    requires std::is_base_of_v<
        internal::poly_empty_impl,
        typename Ty::template interface<internal::poly_empty_impl>>;

    // Ty should declared a alias which is a value_list called impl.
    typename Ty::template impl<
        typename Ty::template interface<internal::poly_empty_impl>>;
    requires is_value_list_v<typename Ty::template impl<
        typename Ty::template interface<internal::poly_empty_impl>>>;
};

template <typename Ty>
concept _poly_has_extend = requires {
    // Ty should declared a class template called extends.
    typename Ty::template extends<internal::poly_empty_impl>;
    requires std::is_base_of_v<
        internal::poly_empty_impl,
        typename Ty::template extends<internal::poly_empty_impl>>;
    typename Ty::template extends<internal::poly_empty_impl>::from;

    // Ty should declared a alias which is a value_list called impl.
    typename Ty::template impl<
        typename Ty::template extends<internal::poly_empty_impl>>;
    requires is_value_list_v<typename Ty::template impl<
        typename Ty::template extends<internal::poly_empty_impl>>>;
};

namespace internal {
template <typename>
struct poly_assert;
template <typename... Tys>
struct poly_assert<type_list<Tys...>> {
    constexpr static bool value = (poly_assert<Tys>::value && ...);
};
template <typename Ty>
struct poly_assert {
    constexpr static bool value = []() {
        if constexpr (_poly_basic_object<Ty>) {
            return true;
        } else if constexpr (_poly_has_extend<Ty>) {
            return poly_assert<Ty>::value;
        } else {
            return false;
        }
    };
};
} // namespace internal

template <typename Ty>
concept _poly_extend_object =
    _poly_has_extend<Ty> &&
    internal::poly_assert<
        typename Ty::template extends<internal::poly_empty_impl>::from>::value;

template <typename Ty>
concept _poly_object = _poly_basic_object<Ty> || _poly_extend_object<Ty>;

template <typename Ty, typename Object>
concept _poly_impl = requires {
    // The template argument `Object` should satisfy _poly_object
    requires _poly_object<Object>;

    // `Ty` should has functions required in Object impl
    typename Object::template impl<std::remove_cvref_t<Ty>>;
} && !requires {
    // The type has a class or alias named 'poly_tag' is already a polymorphic
    // object or the designer does not whish you do this. You should not pass
    // thru this type as a polymorphic implementation.
    typename Ty::poly_tag;
};

template <_poly_object Object>
struct _vtable;

template <_poly_object Object>
requires _poly_basic_object<Object>
struct _vtable<Object> {
    using exlist = type_list<>;
    using interface =
        typename Object::template interface<internal::poly_empty_impl>;
    using exvalues             = value_list<>;
    using vals                 = typename Object::template impl<interface>;
    using values               = value_list_cat_t<exvalues, vals>;
    constexpr static auto size = value_list_size_v<vals>;

private:
    template <typename MemFunc>
    using void_type_t = typename member_function_traits<MemFunc>::void_type;

    template <auto... Vals>
    constexpr static auto _deduce(value_list<Vals...>) noexcept {
        return static_cast<std::tuple<void_type_t<decltype(Vals)>...>*>(
            nullptr);
    }
    constexpr static auto _deduce() noexcept { return _deduce(values{}); }

public:
    using type = std::remove_pointer_t<decltype(_deduce())>;
};

/*! @cond TURN_OFF_DOXYGEN */

template <_poly_object Object>
using vtable = typename _vtable<Object>::type;

/*! @cond TURN_OFF_DOXYGEN */

template <_poly_object Object, _poly_impl<Object> Impl>
struct _vtable_value_list {
    template <typename ValueList>
    struct _static_list;

    template <auto... Vals>
    struct _static_list<value_list<Vals...>> {
        template <auto, typename>
        struct _element;
        template <auto Val, typename Ret, typename Class, typename... Args>
        struct _element<Val, Ret (Class::*)(Args...)> {
            constexpr static Ret (*value)(void*, Args...) = [](void* ptr,
                                                               Args... args) {
                return (static_cast<Class*>(ptr)->*Val)(
                    std::forward<Args>(args)...);
            };
        };
        template <auto Val, typename Ret, typename Class, typename... Args>
        struct _element<Val, Ret (Class::*)(Args...) const> {
            constexpr static Ret (*value)(const void*, Args...) =
                [](const void* ptr, Args... args) {
                    return (static_cast<const Class*>(ptr)->*Val)(
                        std::forward<Args>(args)...);
                };
        };
        template <auto Val, typename Ret, typename Class, typename... Args>
        struct _element<Val, Ret (Class::*)(Args...) noexcept> {
            constexpr static Ret (*value)(void*, Args...) noexcept =
                [](void* ptr, Args... args) noexcept {
                    return (static_cast<Class*>(ptr)->*Val)(
                        std::forward<Args>(args)...);
                };
        };
        template <auto Val, typename Ret, typename Class, typename... Args>
        struct _element<Val, Ret (Class::*)(Args...) const noexcept> {
            constexpr static Ret (*value)(const void*, Args...) noexcept =
                [](const void* ptr, Args... args) noexcept {
                    return (static_cast<const Class*>(ptr)->*Val)(
                        std::forward<Args>(args)...);
                };
        };
        template <auto Val, typename Func>
        constexpr static auto _element_v = _element<Val, Func>::value;

        using type = value_list<_element_v<Vals, decltype(Vals)>...>;
    };

    using type = _static_list<typename Object::template impl<Impl>>::type;
};

template <_poly_object Object, _poly_impl<Object> Impl>
using _vtable_value_list_t = _vtable_value_list<Object, Impl>::type;

/*! @endcond */

template <_poly_object Object, _poly_impl<Object> Impl>
consteval auto make_vtable() noexcept {
    return make_tuple<_vtable_value_list_t<Object, Impl>>();
}

template <_poly_object Object, _poly_impl<Object> Impl>
constexpr static inline auto static_vtable = make_vtable<Object, Impl>();

template <typename Poly>
class poly_base;

// polymorphic<Object> ->  Object::temlate interface<Impl> ->
// polymorphic_base<polymorphic<Object>

template <_poly_object Object, typename>
struct _poly_object_extract;
template <_poly_basic_object Object, typename Arg>
struct _poly_object_extract<Object, Arg> {
    using type = Object::template interface<Arg>;
};
template <_poly_object Object, typename Arg>
using _poly_object_extract_t = typename _poly_object_extract<Object, Arg>::type;

/**
 * @brief Static polymorphic object container
 *
 * Implements runtime polymorphism without traditional vtable overhead
 *
 * @tparam Object Polymorphic object type defining the interface
 * @tparam Size Storage buffer size
 * @tparam Align Storage buffer alignment
 * @tparam Ops Custom operations tuple
 */
template <
    _poly_object Object, size_t Size = k_default_poly_storage_size,
    size_t Align = k_default_poly_storage_align, typename Ops = std::tuple<>>
class poly :
    private _poly_object_extract_t<
        Object, poly_base<poly<Object, Size, Align, Ops>>> {

    // Friend declaration for CRTP

    friend class poly_base<poly>;

    // Internal type aliases

    struct _poly_operation_destroy {
        void (*value)(void* ptr) = nullptr;
    };

    using _empty_interface =
        _poly_object_extract_t<Object, internal::poly_empty_impl>;
    using vtable_t = vtable<Object>;
    using ops_t =
        decltype(std::tuple_cat(Ops{}, std::tuple<_poly_operation_destroy>{}));
    using size_type = uint32_t;

    // Storage management

    constexpr static bool _store_vtable_as_pointer = sizeof(vtable_t) >
                                                     sizeof(void*) * 2;

    ///< Pointer to stored object
    void* ptr_;

    /// Hash code of the polymorphic object.
    size_t hash_code_;

    /// The vtable for the polymorphic object.
    std::conditional_t<_store_vtable_as_pointer, const vtable_t*, vtable_t>
        vtable_;

    ///< Custom operations
    ops_t operations_;

    // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
    // NOLINTBEGIN(modernize-avoid-c-arrays)

    /// Internal storage buffer
    /// @note The minimum size of the storage buffer is the size of 'size_type'
    /// to store the size of the object when the pointer refers to heap memory.
    alignas(Align)
        std::byte storage_[Size > sizeof(size_type) ? Size : sizeof(size_type)];

    // NOLINTEND(modernize-avoid-c-arrays)
    // NOLINTEND(cppcoreguidelines-avoid-c-arrays)

    /// @brief return compile-time virtual table value.
    template <_poly_impl<Object> Impl>
    consteval static auto _vtable_value() noexcept {
        if constexpr (_store_vtable_as_pointer) {
            return &static_vtable<Object, Impl>;
        } else {
            return make_vtable<Object, Impl>();
        }
    }

    template <_poly_impl<Object> Impl>
    constexpr static bool builtin_storable =
        sizeof(Impl) <= Size && alignof(Impl) <= Align;

    template <_poly_impl<Object> Impl>
    constexpr void* _init_ptr() {
        if constexpr (builtin_storable<Impl>) {
            return static_cast<void*>(storage_);
        } else {
            *static_cast<size_type*>(storage_) = sizeof(Impl);
            return operator new(sizeof(Impl), std::align_val_t(Align));
        }
    }

    template <_poly_impl<Object> Impl>
    constexpr void _construct(Impl&& impl) {
        ::new (ptr_) Impl(std::forward<Impl>(impl));
        ptr_ = std::launder(static_cast<Impl*>(ptr_));
    }

    constexpr void _destroy_without_reset() {
        get_first<_poly_operation_destroy>(operations_).value(ptr_);
    }

    constexpr void _destroy() {
        auto& operation = get_first<_poly_operation_destroy>(operations_);
        operation.value(ptr_);
        ptr_            = nullptr;
        operation.value = nullptr;
    }

    constexpr void _init_ptr_from(const poly& that) {
        if (that.ptr_ == &that.storage_) {
            ptr_ = &storage_;
        } else {
            const auto size = *static_cast<size_type*>(that.storage_);
            *static_cast<size_type*>(storage_) = size;
            ptr_ = ::operator new(size, std::align_val_t(Align));
        }
    }

    constexpr void _init_ptr_from(poly&& that) {
        if (that.ptr_ == &that.storage_) {
            ptr_ = &storage_;
        } else {
            *static_cast<size_type*>(storage_) =
                *static_cast<size_type*>(std::move(that).storage_);
            ptr_ = std::exchange(that.ptr_, nullptr);
        }
    }

    template <_poly_impl<Object> Impl>
    constexpr static ops_t _operations() noexcept {
        ops_t ops;
        if constexpr (type_list_has_v<poly_op_default_construct, ops_t>) {
            get_first<poly_op_default_construct>(ops).value =
                [](void* ptr) noexcept(
                    std::is_nothrow_default_constructible_v<Impl>) {
                    ::new (ptr) Impl();
                };
        }
        if constexpr (type_list_has_v<poly_op_copy_construct, ops_t>) {
            get_first<poly_op_copy_construct>(ops).value =
                [](void* lhs, const void* rhs) noexcept(
                    std::is_nothrow_copy_constructible_v<Impl>) {
                    ::new (lhs) Impl(*static_cast<const Impl*>(rhs));
                };
        }
        if constexpr (type_list_has_v<poly_op_move_construct, ops_t>) {
            get_first<poly_op_move_construct>(ops).value =
                [](void* lhs, void* rhs) noexcept(
                    std::is_nothrow_move_constructible_v<Impl>) {
                    ::new (lhs) Impl(std::move(*static_cast<Impl*>(rhs)));
                };
        }
        if constexpr (type_list_has_v<poly_op_copy_assign, ops_t>) {
            get_first<poly_op_copy_assign>(ops).value =
                [](void* lhs, const void* rhs) noexcept(
                    std::is_nothrow_copy_assignable_v<Impl>) {
                    *static_cast<Impl*>(lhs) = *static_cast<const Impl*>(rhs);
                };
        }
        if constexpr (type_list_has_v<poly_op_move_assign, ops_t>) {
            get_first<poly_op_move_assign>(ops).value =
                [](void* lhs, void* rhs) noexcept(
                    std::is_nothrow_move_assignable_v<Impl>) {
                    *static_cast<Impl*>(lhs) =
                        std::move(*static_cast<Impl*>(rhs));
                };
        }
        get_first<_poly_operation_destroy>(ops).value =
            [](void* ptr) noexcept(std::is_nothrow_destructible_v<Impl>) {
                static_cast<Impl*>(ptr)->~Impl();
            };
        return ops;
    }

    constexpr void _assign_context(const poly& that) {
        hash_code_  = that.hash_code_;
        vtable_     = that.vtable_;
        operations_ = that.operations_;
    }

public:
    using object_type = Object;
    using vtable_type = vtable_t;
    using interface   = _poly_object_extract_t<Object, poly_base<poly>>;

    /**
     * @brief Constructs a polymorphic object with an empty interface.
     * @warning This constructor initializes the polymorphic object with an
     * empty interface. You should construct the polymorphic object with a valid
     * implementation before using it.
     */
    constexpr poly() noexcept
        : ptr_(nullptr), hash_code_(),
          vtable_(_vtable_value<_empty_interface>()), operations_(),
          storage_() {}

    template <_poly_impl<Object> Impl>
    constexpr poly(Impl&& impl)
        : ptr_(_init_ptr<Impl>()), hash_code_(hash_of<Impl>()),
          vtable_(_vtable_value<Impl>()), operations_(_operations<Impl>()),
          storage_() {
        _construct(std::forward<Impl>(impl));
    }

    constexpr poly(const poly& that)
    requires type_list_has_v<poly_op_copy_construct, ops_t>
        : ptr_(nullptr), hash_code_(that.hash_code_), vtable_(that.vtable_),
          operations_(that.operations_), storage_() {
        if (that.ptr_ != nullptr) {
            _init_ptr_from(that);
            const auto operation =
                get_first<poly_op_copy_construct>(operations_);
            operation.value(ptr_, that.ptr_);
        }
    }

    constexpr poly(poly&& that) noexcept
    requires type_list_has_v<poly_op_move_construct, ops_t>
        : ptr_(nullptr), hash_code_(std::exchange(that.hash_code_, 0)),
          vtable_(that.vtable_), storage_(), operations_(that.operations_) {
        if (that.ptr_ != nullptr) {
            _init_ptr_from(std::move(that));
            const auto operation =
                get_first<poly_op_move_construct>(operations_);
            operation.value(ptr_, that.ptr_);
        }
    }

    template <_poly_impl<Object> Impl>
    constexpr poly& operator=(Impl&& that) {
        constexpr auto hash_code = hash_of<Impl>();
        if (hash_code_ != hash_code) {
            if (ptr_) {
                _destroy();
            }
            _init_ptr<Impl>();
            _construct(std::forward<Impl>(that));
            hash_code_  = hash_code;
            vtable_     = _vtable_value<Impl>();
            operations_ = _operations<Impl>();
        } else {
            *static_cast<Impl*>(ptr_) = std::forward<Impl>(that);
        }
    }

private:
    constexpr void _make_same(const poly& that) {
        _init_ptr_from(that);
        if constexpr (type_list_has_v<poly_op_copy_construct, ops_t>) {
            get_first<poly_op_copy_construct>(operations_)
                .value(ptr_, that.ptr_);
        } else if constexpr (
            type_list_has_v<poly_op_default_construct, ops_t> &&
            type_list_has_v<poly_op_copy_assign, ops_t>) {
            get_first<poly_op_default_construct>(operations_).value(ptr_);
            get_first<poly_op_copy_assign>(operations_).value(ptr_, that.ptr_);
        }
        _assign_context(that);
    }

public:
    constexpr poly& operator=(const poly& that)
    requires(type_list_has_v<poly_op_copy_assign, ops_t> &&
             type_list_has_v<poly_op_default_construct, ops_t>) ||
            type_list_has_v<poly_op_copy_construct, ops_t>
    {
        if (this == &that) [[unlikely]] {
            return *this;
        }

        if (that.ptr_ != nullptr) {
            if (ptr_ != nullptr) {
                if (hash_code_ == that.hash_code_) {
                    if constexpr (type_list_has_v<poly_op_copy_assign, ops_t>) {
                        get_first<poly_op_copy_assign>(operations_)
                            .value(ptr_, that.ptr_);
                    } else if constexpr (type_list_has_v<
                                             poly_op_copy_construct, ops_t>) {
                        _destroy_without_reset();
                        get_first<poly_op_copy_construct>(operations_)
                            .value(ptr_, that.ptr_);
                    }
                    return *this;
                }
                _destroy_without_reset();
            }
            _make_same(that);
        } else {
            if (ptr_ != nullptr) {
                _destroy();
            }
        }

        return *this;
    }

private:
    constexpr void _make_same(poly&& that) {
        _init_ptr_from(std::move(that));
        if constexpr (type_list_has_v<poly_op_move_construct, ops_t>) {
            get_first<poly_op_move_construct>(operations_)
                .value(ptr_, that.ptr_);
        } else if constexpr (
            type_list_has_v<poly_op_default_construct, ops_t> &&
            type_list_has_v<poly_op_move_assign, ops_t>) {
            get_first<poly_op_default_construct>(operations_).value(ptr_);
            get_first<poly_op_move_assign>(operations_).value(ptr_, that.ptr_);
        }
        _assign_context(that);
    }

public:
    constexpr poly& operator=(poly&& that) noexcept
    requires type_list_has_v<poly_op_move_assign, ops_t> ||
             type_list_has_v<poly_op_move_construct, ops_t>
    {
        if (this == &that) [[unlikely]] {
            return *this;
        }

        if (that.ptr_ != nullptr) {
            if (ptr_ != nullptr) {
                if (hash_code_ == that.hash_code_) {
                    if constexpr (type_list_has_v<poly_op_move_assign, ops_t>) {
                        get_first<poly_op_move_assign>(operations_)
                            .value(ptr_, that.ptr_);
                    } else if constexpr (type_list_has_v<
                                             poly_op_move_construct, ops_t>) {
                        _destroy_without_reset();
                        get_first<poly_op_move_construct>(operations_)
                            .value(ptr_, that.ptr_);
                    }
                    return *this;
                }
                _destroy_without_reset();
            }
            _make_same(std::move(that));
        } else {
            if (ptr_ != nullptr) {
                _destroy();
            }
        }

        return *this;
    }

    constexpr ~poly() {
        if (ptr_ != nullptr) [[likely]] {
            _destroy();
            ptr_ = nullptr;
        }
    }

    [[nodiscard]] constexpr interface* operator->() noexcept { return this; }
    [[nodiscard]] constexpr const interface* operator->() const noexcept {
        return this;
    }

    [[nodiscard]] constexpr operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    [[nodiscard]] constexpr void* data() noexcept { return ptr_; }
    [[nodiscard]] constexpr const void* data() const noexcept { return ptr_; }
};

template <
    _poly_object Object, _poly_impl<Object> Impl,
    size_t Size  = k_default_poly_storage_size,
    size_t Align = k_default_poly_storage_align, typename Ops = std::tuple<>,
    typename... Args>
requires std::constructible_from<Impl, Args...>
constexpr poly<Object, Size, Align, Ops> make_poly(Args&&... args) {
    // TODO:
}

template <typename Poly>
class poly_base;

template <_poly_object Object, size_t Size, size_t Align, typename Ops>
class poly_base<poly<Object, Size, Align, Ops>> {
public:
    using polymorphic_type = poly<Object, Size, Align, Ops>;

    template <size_t Index, _poly_object O = Object, typename... Args>
    constexpr decltype(auto) invoke(Args&&... args) {
        constexpr size_t off = value_list_size_v<typename _vtable<O>::exvalues>;
        auto* const ptr      = static_cast<polymorphic_type*>(this);
        if constexpr (polymorphic_type::_store_vtable_as_pointer) {
            return std::get<off + Index>(*(ptr->vtable_))(
                ptr->ptr_, std::forward<Args>(args)...);
        } else {
            return std::get<off + Index>(ptr->vtable_)(
                ptr->ptr_, std::forward<Args>(args)...);
        }
    }

    template <size_t Index, _poly_object O = Object, typename... Args>
    constexpr decltype(auto) invoke(Args&&... args) const {
        constexpr size_t off = value_list_size_v<typename _vtable<O>::exvalues>;
        auto* const ptr      = static_cast<const polymorphic_type*>(this);
        if constexpr (polymorphic_type::_store_vtable_as_pointer) {
            return std::get<off + Index>(*(ptr->vtable_))(
                ptr->ptr_, std::forward<Args>(args)...);
        } else {
            return std::get<off + Index>(ptr->vtable_)(
                ptr->ptr_, std::forward<Args>(args)...);
        }
    }
};

} // namespace neutron
