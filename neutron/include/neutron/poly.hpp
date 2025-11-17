#pragma once
#include <cstddef>
#include <cstdlib>
#include <new>
#include <utility>
#include "neutron/template_list.hpp"
#include "neutron/type_hash.hpp"
#include "../src/neutron/internal/exception_guard.hpp"
#include "../src/neutron/internal/poly/vtable.hpp"
#include "../src/neutron/internal/spreader.hpp"

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

template <poly_object Object, poly_impl<Object> Impl>
constexpr static inline auto static_vtable = make_vtable<Object, Impl>();

template <typename Poly>
class poly_base;

// polymorphic<Object> ->  Object::temlate interface<Impl> ->
// polymorphic_base<polymorphic<Object>

template <poly_object Object, typename Arg>
struct _poly_object_extract {
    using type = Object::template interface<Arg>;
};
template <poly_object Object, typename Arg>
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
    poly_object Object, size_t Size = k_default_poly_storage_size,
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

    ///< Pointer to stored object
    void* ptr_;

    /// Hash code of the polymorphic object.
    size_t hash_code_;

    /// The vtable for the polymorphic object.
    vtable<Object> vtable_;

    ///< Custom operations
    ops_t ops_;

    // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
    // NOLINTBEGIN(modernize-avoid-c-arrays)

    /// Internal storage buffer
    /// @note The minimum size of the storage buffer is the size of 'size_type'
    /// to store the size of the object when the pointer refers to heap memory.
    alignas(Align)
        std::byte storage_[Size > sizeof(size_type) ? Size : sizeof(size_type)];

    // NOLINTEND(modernize-avoid-c-arrays)
    // NOLINTEND(cppcoreguidelines-avoid-c-arrays)

    template <poly_impl<Object> Impl>
    constexpr static bool builtin_storable =
        sizeof(Impl) <= Size && alignof(Impl) <= Align;

    template <poly_impl<Object> Impl>
    constexpr void* _init_ptr() {
        if constexpr (builtin_storable<Impl>) {
            return static_cast<void*>(storage_);
        } else {
            *static_cast<size_type*>(storage_) = sizeof(Impl);
            return operator new(sizeof(Impl), std::align_val_t(Align));
        }
    }

    template <poly_impl<Object> Impl>
    constexpr void _construct(Impl&& impl) {
        ::new (ptr_) Impl(std::forward<Impl>(impl));
        ptr_ = std::launder(static_cast<Impl*>(ptr_));
    }

    constexpr void _destroy_without_reset() {
        get_first<_poly_operation_destroy>(ops_).value(ptr_);
    }

    constexpr void _destroy() {
        auto& operation = get_first<_poly_operation_destroy>(ops_);
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

    template <poly_impl<Object> Impl>
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
        hash_code_ = that.hash_code_;
        vtable_    = that.vtable_;
        ops_       = that.ops_;
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
        : ptr_(nullptr), hash_code_(), vtable_(spread_type<_empty_interface>),
          ops_(), storage_() {}

    template <poly_impl<Object> Impl>
    constexpr poly(Impl&& impl)
        : ptr_(_init_ptr<Impl>()), hash_code_(hash_of<Impl>()),
          vtable_(spread_type<Impl>), ops_(_operations<Impl>()), storage_() {
        auto guard = make_exception_guard([this] { _destroy(); });
        _construct(std::forward<Impl>(impl));
        guard.mark_complete();
    }

    constexpr poly(const poly& that)
    requires type_list_has_v<poly_op_copy_construct, ops_t>
        : ptr_(nullptr), hash_code_(that.hash_code_), vtable_(that.vtable_),
          ops_(that.ops_), storage_() {
        if (that.ptr_ != nullptr) {
            _init_ptr_from(that);
            const auto operation = get_first<poly_op_copy_construct>(ops_);
            operation.value(ptr_, that.ptr_);
        }
    }

    constexpr poly(poly&& that) noexcept
    requires type_list_has_v<poly_op_move_construct, ops_t>
        : ptr_(nullptr), hash_code_(std::exchange(that.hash_code_, 0)),
          vtable_(that.vtable_), storage_(), ops_(that.ops_) {
        if (that.ptr_ != nullptr) {
            _init_ptr_from(std::move(that));
            const auto operation = get_first<poly_op_move_construct>(ops_);
            operation.value(ptr_, that.ptr_);
        }
    }

    template <poly_impl<Object> Impl>
    constexpr poly& operator=(Impl&& that) {
        constexpr auto hash_code = hash_of<Impl>();
        if (hash_code_ != hash_code) {
            if (ptr_) {
                _destroy();
            }
            _init_ptr<Impl>();
            _construct(std::forward<Impl>(that));
            hash_code_ = hash_code;
            vtable_    = _vtable_value<Impl>();
            ops_       = _operations<Impl>();
        } else {
            *static_cast<Impl*>(ptr_) = std::forward<Impl>(that);
        }
    }

private:
    constexpr void _make_same(const poly& that) {
        _init_ptr_from(that);
        if constexpr (type_list_has_v<poly_op_copy_construct, ops_t>) {
            get_first<poly_op_copy_construct>(ops_).value(ptr_, that.ptr_);
        } else if constexpr (
            type_list_has_v<poly_op_default_construct, ops_t> &&
            type_list_has_v<poly_op_copy_assign, ops_t>) {
            get_first<poly_op_default_construct>(ops_).value(ptr_);
            get_first<poly_op_copy_assign>(ops_).value(ptr_, that.ptr_);
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
                        get_first<poly_op_copy_assign>(ops_).value(
                            ptr_, that.ptr_);
                    } else if constexpr (type_list_has_v<
                                             poly_op_copy_construct, ops_t>) {
                        _destroy_without_reset();
                        get_first<poly_op_copy_construct>(ops_).value(
                            ptr_, that.ptr_);
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
            get_first<poly_op_move_construct>(ops_).value(ptr_, that.ptr_);
        } else if constexpr (
            type_list_has_v<poly_op_default_construct, ops_t> &&
            type_list_has_v<poly_op_move_assign, ops_t>) {
            get_first<poly_op_default_construct>(ops_).value(ptr_);
            get_first<poly_op_move_assign>(ops_).value(ptr_, that.ptr_);
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
                        get_first<poly_op_move_assign>(ops_).value(
                            ptr_, that.ptr_);
                    } else if constexpr (type_list_has_v<
                                             poly_op_move_construct, ops_t>) {
                        _destroy_without_reset();
                        get_first<poly_op_move_construct>(ops_).value(
                            ptr_, that.ptr_);
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
    poly_object Object, poly_impl<Object> Impl,
    size_t Size  = k_default_poly_storage_size,
    size_t Align = k_default_poly_storage_align, typename Ops = std::tuple<>,
    typename... Args>
requires std::constructible_from<Impl, Args...>
constexpr poly<Object, Size, Align, Ops> make_poly(Args&&... args) {
    // TODO:
}

template <typename Poly>
class poly_base;

template <poly_object Object, size_t Size, size_t Align, typename Ops>
class poly_base<poly<Object, Size, Align, Ops>> {
public:
    using polymorphic_type = poly<Object, Size, Align, Ops>;

    template <size_t Index, poly_object O = Object, typename... Args>
    constexpr decltype(auto) invoke(Args&&... args) const {
        constexpr size_t off = 0;
        auto* const ptr      = static_cast<const polymorphic_type*>(this);
        return (ptr->vtable_.template get<off + Index>())(
            ptr->ptr_, std::forward<Args>(args)...);
    }
};

} // namespace neutron
