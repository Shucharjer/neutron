#pragma once
#include <initializer_list>

#if defined(__i386__) || defined(__x86_64__)
    #define TARGET_X86
    #include <immintrin.h>
#elif defined(__arm__) || defined(__aarch64__) || defined(__arm64ec__)
    #define TARGET_ARM
    #include <arm_neon.h>
#elif defined(__riscv)
    #define TARGET_RISCV
    #include <riscv_vector.h>
#else
    #error "not supported yet"
#endif

#include <version>

// Compiler-specific function name macros

#ifdef _MSC_VER
    #define ATOM_FUNCNAME __FUNCSIG__         ///< MSVC function signature macro
#else
    #define ATOM_FUNCNAME __PRETTY_FUNCTION__ ///< GCC/Clang function name macro
#endif

// Compiler-specific attribute macros

#if defined(__GNUC__) || defined(__clang__)
    #define ATOM_FORCE_INLINE                                                                      \
        inline __attribute__((__always_inline__))       ///< Force inline (GCC/Clang)
    #define ATOM_NOINLINE __attribute__((__noinline__)) ///< Prevent inlining (GCC/Clang)
    #define ATOM_ALLOCATOR __attribute__((malloc))      ///< Returns allocated memory (GCC/Clang)
    #define ATOM_NOVTABLE                               ///< Empty for GCC/Clang
#elif defined(_MSC_VER)
    #define ATOM_FORCE_INLINE __forceinline             ///< Force inline expansion (MSVC)
    #define ATOM_FORCE_NOINLINE __declspec(noinline)    ///< Prevent inlining (MSVC)
    #define ATOM_ALLOCATOR __declspec(allocator) ///< Function returns allocated memory (MSVC)
    #define ATOM_NOVTABLE __declspec(novtable)   ///< No vtable generation (MSVC)
#else
    #define ATOM_FORCE_INLINE inline             ///< Fallback inline
    #define ATOM_NOINLINE                        ///< Fallback noinline
    #define ATOM_ALLOCATOR                       ///< Fallback allocator
    #define ATOM_NOVTABLE                        ///< Fallback novtable
#endif

#ifdef _MSC_VER
    #define ATOM_NO_UNIQUE_ADDR [[msvc::no_unique_address]]
#else
    #define ATOM_NO_UNIQUE_ADDR [[no_unique_address]]
#endif

// Debug/Release configuration

#if defined(_DEBUG)
    #define ATOM_RELEASE_INLINE                                ///< No forced inlining in debug
    #define ATOM_DEBUG_SHOW_FUNC                                                                   \
        constexpr std::string_view _this_func = ATOM_FUNCNAME; ///< Store function name
#else
    #define ATOM_RELEASE_INLINE ATOM_FORCE_INLINE              ///< Force inline in release
    #define ATOM_DEBUG_SHOW_FUNC                               ///< Empty in release
#endif

#if defined(__i386__) || defined(__x86_64__)
    #define ATOM_VECTORIZABLE true
#else
    #define ATOM_VECTORIZABLE false
#endif

// C++ standard version detection

#if defined(_HAS_CXX17) && !defined(HAS_CXX17)
    #define HAS_CXX17 _HAS_CXX17             ///< C++17 supported
    #if defined(_HAS_CXX20) && !defined(HAS_CXX20)
        #define HAS_CXX20 _HAS_CXX20         ///< C++20 supported
        #if defined(_HAS_CXX23) && !defined(HAS_CXX23)
            #define HAS_CXX23 _HAS_CXX23     ///< C++23 supported
            #if defined(_HAS_CXX26) && !defined(HAS_CXX26)
                #define HAS_CXX26 _HAS_CXX26 ///< C++26 supported
            #endif
        #endif
    #endif
#endif

#ifndef HAS_CXX20
    #if __cplusplus > 201703L
        #define HAS_CXX20 1 ///< C++20 supported
    #else
        #error "this library is based on c++20"
    #endif
#endif

#ifndef HAS_CXX23
    #if HAS_CXX20 && __cplusplus > 202002L
        #define HAS_CXX23 1 ///< C++23 supported
    #else
        #define HAS_CXX23 0 ///< C++23 not supported
    #endif
#endif

#ifndef HAS_CXX26
    #if HAS_CXX23 && __cplusplus > 202302L
        #define HAS_CXX26 1 ///< C++26 supported
    #else
        #define HAS_CXX26 0 ///< C++26 not supported
    #endif
#endif

// Constexpr macros per C++ standard

#ifndef CONSTEXPR20
    #if HAS_CXX20
        #define CONSTEXPR20 constexpr ///< C++20 constexpr
    #else
        #define CONSTEXPR20           ///< Empty for older standards
    #endif
#endif

#ifndef CONSTEXPR23
    #if HAS_CXX23
        #define CONSTEXPR23 constexpr ///< C++23 constexpr
    #else
        #define CONSTEXPR23           ///< Empty for older standards
    #endif
#endif

#ifndef CONSTEXPR26
    #if HAS_CXX26
        #define CONSTEXPR26 constexpr ///< C++26 constexpr
    #else
        #define CONSTEXPR26           ///< Empty for older standards
    #endif
#endif

// Static keyword for C++23
#ifndef STATIC23
    #if HAS_CXX23
        #define STATIC23 static ///< C++23 static
    #else
        #define STATIC23        ///< Empty for older standards
    #endif
#endif

// [[nodiscard]] attribute support
#ifndef HAS_NODISCARD
    #if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard) >= 201603L
        #define HAS_NODISCARD 1 ///< [[nodiscard]] supported
    #else
        #define HAS_NODISCARD 0 ///< [[nodiscard]] not supported
    #endif
#endif

#if HAS_NODISCARD
    #define NODISCARD [[nodiscard]] ///< Attribute to warn on unused return values
#else
    #define NODISCARD               ///< Empty when not supported
#endif

#ifndef HAS_INDETERMINATE
    #if defined(__has_cpp_attribute) && __has_cpp_attribute(indeterminate) > 202303L
        #define HAS_INDETERMINATE 1
    #else
        #define HAS_INDETERMINATE 0
    #endif
#endif

#if HAS_INDETERMINATE
    #define INDETERMINATE [[indeterminate]]
#else
    #define INDETERMINATE
#endif

#include <cstdint>
#include <utility>

#if __cplusplus >= 202302L
    #define CONST_EVALUATED consteval
#else
    #define CONST_EVALUATED (std::is_constant_evaluated())
#endif

namespace neutron {

#ifndef UTILS_LONG_ID
using id_t = uint32_t;
#else
using id_t = uint64_t;
#endif

/**
 * @brief Template helper for passing non-type template arguments
 *
 * @tparam auto Argument value to pass
 *
 * Usage:
 *   func(spread_arg<42>);
 */
template <auto...>
struct spreader {
    explicit spreader() = default;
};

template <auto... Candidate>
inline constexpr spreader<Candidate...> spread_arg{}; ///< Global instance

/**
 * @brief Template helper for passing type arguments
 *
 * @tparam typename Type to pass
 *
 * Usage:
 *   func(spread_type<int>);
 */
template <typename...>
struct type_spreader {
    explicit type_spreader() = default;
};

template <typename ...Tys>
inline constexpr type_spreader<Tys...> spread_type{}; ///< Global instance

// in identity.hpp
class type_identity;
class non_type_identity;

/**
 * @class delegate
 * @brief A thin wrapper of function pointer and context.
 * It prefers performance over usability. We can only bind it by template non-type arugment, which
 * means it could be inlined by compiler as much as possible.
 * @tparam Ret Return type of the function.
 * @tparam Args Arguments of the function.
 */
template <typename = void()>
class delegate;

template <typename = void>
class coroutine;

template <typename = void>
class lazy_routine;

#if _MSC_VER
    #include <vcruntime.h> // for _HAS_EXCEPTIONS
#endif
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || (_HAS_EXCEPTIONS)
    #define ENABLED_CPP_EXCEPTIONS true
#else
    #define ENABLED_CPP_EXCEPTIONS false
#endif

#if ENABLED_CPP_EXCEPTIONS

template <typename Rollback>
class exception_guard {

    template <typename Roll>
    constexpr exception_guard(Roll&& rollback) noexcept : rollback_(std::forward<Roll>(rollback)) {}

    template <typename Roll>
    friend constexpr auto make_exception_guard(Roll&&) noexcept
        -> exception_guard<std::remove_cvref_t<Roll>>;

public:
    constexpr exception_guard(const exception_guard&)            = delete;
    constexpr exception_guard(exception_guard&&)                 = delete;
    constexpr exception_guard& operator=(const exception_guard&) = delete;
    constexpr exception_guard& operator=(exception_guard&&)      = delete;

    constexpr void mark_complete() noexcept { complete_ = true; }

    constexpr ~exception_guard() noexcept {
        if (!complete_) [[unlikely]] {
            rollback_();
        }
    }

private:
    bool complete_{ false };
    Rollback rollback_;
};

template <typename Roll>
NODISCARD constexpr auto make_exception_guard(Roll&& rollback) noexcept
    -> exception_guard<std::remove_cvref_t<Roll>> {
    return exception_guard<std::remove_cvref_t<Roll>>(std::forward<Roll>(rollback));
}

#else

class _empty_exception_guard {
    constexpr exception_guard() noexcept = default;

    template <typename Rollback>
    friend constexpr auto make_exception_guard(Rollback&& rollback) noexcept
        -> _empty_exception_guard;

public:
    constexpr exception_guard(const exception_guard&)            = delete;
    constexpr exception_guard(exception_guard&&)                 = delete;
    constexpr exception_guard& operator=(const exception_guard&) = delete;
    constexpr exception_guard& operator=(exception_guard&&)      = delete;

    constexpr void mark_complete() noexcept {}

    constexpr ~exception_guard() noexcept = default;
};

template <typename Rollback>
NODISCARD auto make_exception_guard(Rollback&& rollback) noexcept -> _empty_exception_guard {
    return _empty_exception_guard{};
}

#endif

struct immediately_t {};
constexpr inline immediately_t immediately{};

template <typename Ty1, typename Ty2>
concept _not = (!std::same_as<Ty1, Ty2>);

template <typename Ty, typename... Args>
consteval auto make_array(Args&&... args) {
    return std::array<Ty, sizeof...(Args)>{ Ty{ std::forward<Args>(args) }... };
}

} // namespace neutron

using neutron::make_array;

using neutron::id_t;
