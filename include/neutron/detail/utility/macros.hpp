// IWYU pragma: private
#pragma once

// Only macros: the headers of neutron include this one in every mode,
// including the importers of the `neutron` module, which must not see any
// declaration of the standard library before `import std;`.
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) ||            \
    defined(_M_X64)
    #define ATOM_TARGET_X86
#elif defined(__arm__) || defined(__aarch64__) || defined(__arm64ec__)
    #define ATOM_TARGET_ARM
#else
    #error "not supported yet"
#endif

#include <version>

// Modules

/// @brief Exports a declaration from the `neutron` module.
///
/// `ATOM_BUILDING_MODULE` is defined only by the module interface unit of
/// neutron, which also provides the standard library through `import std;`.
/// Every other declaration of neutron is internal to the module.
#ifdef ATOM_BUILDING_MODULE
    #define ATOM_EXPORT export
#else
    #define ATOM_EXPORT
#endif

// Feature-detection fallbacks for non-clang compilers
#ifndef __has_feature
    #define __has_feature(x) 0
#endif
#ifndef __has_builtin
    #define __has_builtin(x) 0
#endif

// Compiler-specific function name macros

#ifdef _MSC_VER
    #define ATOM_FUNCNAME __FUNCSIG__         ///< MSVC function signature macro
#else
    #define ATOM_FUNCNAME __PRETTY_FUNCTION__ ///< GCC/Clang function name macro
#endif

// Compiler-specific attribute macros

#if defined(__GNUC__) || defined(__clang__)
    #define ATOM_ALLOCATOR                                                     \
        __attribute__((malloc)) ///< Returns allocated memory (GCC/Clang)
    #define ATOM_NOVTABLE       ///< Empty for GCC/Clang
#elif defined(_MSC_VER)
    #define ATOM_ALLOCATOR                                                     \
        __declspec(allocator) ///< Function returns allocated memory (MSVC)
    #define ATOM_NOVTABLE __declspec(novtable) ///< No vtable generation (MSVC)
#else
    #define ATOM_ALLOCATOR                     ///< Fallback allocator
    #define ATOM_NOVTABLE                      ///< Fallback novtable
#endif

#ifdef _MSC_VER
    #define ATOM_NO_UNIQUE_ADDR [[msvc::no_unique_address]]
    /// MSVC only applies the empty base optimization to the first base by
    /// default.
    #define ATOM_EMPTY_BASES __declspec(empty_bases)
#else
    #define ATOM_NO_UNIQUE_ADDR [[no_unique_address]]
    #define ATOM_EMPTY_BASES
#endif

// Debug/Release configuration

#if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
    #define ATOM_DEBUG_MODE true
    #define ATOM_RELEASE_INLINE ///< No forced inlining in debug
    #define ATOM_DEBUG_SHOW_FUNC                                               \
        constexpr std::string_view _this_func =                                \
            ATOM_FUNCNAME; ///< Store function name
#else
    #define ATOM_DEBUG_MODE false
    #define ATOM_RELEASE_INLINE ATOM_ALWAYS_INLINE ///< Force inline in release
    #define ATOM_DEBUG_SHOW_FUNC                   ///< Empty in release
#endif

#if defined(__i386__) || defined(__x86_64__)
    #define ATOM_VECTORIZABLE true
#else
    #define ATOM_VECTORIZABLE false
#endif

// C++ standard version detection

#if defined(_HAS_CXX17) && !defined(ATOM_HAS_CXX17)
    #define ATOM_HAS_CXX17 _HAS_CXX17             ///< C++17 supported
    #if defined(_HAS_CXX20) && !defined(ATOM_HAS_CXX20)
        #define ATOM_HAS_CXX20 _HAS_CXX20         ///< C++20 supported
        #if defined(_HAS_CXX23) && !defined(ATOM_HAS_CXX23)
            #define ATOM_HAS_CXX23 _HAS_CXX23     ///< C++23 supported
            #if defined(_HAS_CXX26) && !defined(ATOM_HAS_CXX26)
                #define ATOM_HAS_CXX26 _HAS_CXX26 ///< C++26 supported
            #endif
        #endif
    #endif
#endif

#ifndef ATOM_HAS_CXX20
    #if __cplusplus > 201703L
        #define ATOM_HAS_CXX20 1 ///< C++20 supported
    #else
        #error "this library is based on c++20"
    #endif
#endif

#ifndef ATOM_HAS_CXX23
    #if ATOM_HAS_CXX20 && __cplusplus > 202002L
        #define ATOM_HAS_CXX23 1 ///< C++23 supported
    #else
        #define ATOM_HAS_CXX23 0 ///< C++23 not supported
    #endif
#endif

#ifndef ATOM_HAS_CXX26
    #if ATOM_HAS_CXX23 && __cplusplus > 202302L
        #define ATOM_HAS_CXX26 1 ///< C++26 supported
    #else
        #define ATOM_HAS_CXX26 0 ///< C++26 not supported
    #endif
#endif

// Constexpr macros per C++ standard

#ifndef ATOM_CONSTEXPR_SINCE_CXX20
    #if ATOM_HAS_CXX20
        #define ATOM_CONSTEXPR_SINCE_CXX20 constexpr ///< C++20 constexpr
    #else
        #define ATOM_CONSTEXPR_SINCE_CXX20 ///< Empty for older standards
    #endif
#endif

#ifndef ATOM_CONSTEXPR_SINCE_CXX23
    #if ATOM_HAS_CXX23
        #define ATOM_CONSTEXPR_SINCE_CXX23 constexpr ///< C++23 constexpr
    #else
        #define ATOM_CONSTEXPR_SINCE_CXX23 ///< Empty for older standards
    #endif
#endif

#ifndef ATOM_CONSTEXPR_SINCE_CXX26
    #if ATOM_HAS_CXX26
        #define ATOM_CONSTEXPR_SINCE_CXX26 constexpr ///< C++26 constexpr
    #else
        #define ATOM_CONSTEXPR_SINCE_CXX26 ///< Empty for older standards
    #endif
#endif

// Static keyword for C++23
#ifndef ATOM_STATIC_SINCE_CXX23
    #if ATOM_HAS_CXX23
        #define ATOM_STATIC_SINCE_CXX23 static ///< C++23 static
    #else
        #define ATOM_STATIC_SINCE_CXX23        ///< Empty for older standards
    #endif
#endif

// [[nodiscard]] attribute support
#ifndef ATOM_NODISCARD
    #define ATOM_NODISCARD nodiscard
#endif

#ifndef ATOM_HAS_INDETERMINATE
    #if defined(__has_cpp_attribute) &&                                        \
        __has_cpp_attribute(indeterminate) > 202303L
        #define ATOM_HAS_INDETERMINATE 1
    #else
        #define ATOM_HAS_INDETERMINATE 0
    #endif
#endif

#ifndef ATOM_NORETURN
    #define ATOM_NORETURN [[noreturn]]
#endif

#ifndef ATOM_INDETERMINATE
    #if HAS_INDETERMINATE
        #define ATOM_INDETERMINATE [[indeterminate]]
    #else
        #define ATOM_INDETERMINATE
    #endif
#endif

#ifndef ATOM_CONST_EVALUATED
    #if __cplusplus >= 202302L
        #define ATOM_CONST_EVALUATED consteval
    #else
        #define ATOM_CONST_EVALUATED (std::is_constant_evaluated())
    #endif
#endif

#ifndef ATOM_DEPRECATED_SINCE_CXX26
    #if ATOM_HAS_CXX26
        #define ATOM_DEPRECATED_SINCE_CXX26 [[deprecated]]
    #else
        #define ATOM_DEPRECATED_SINCE_CXX26
    #endif
#endif

#ifndef ATOM_HAS_REFLECTION
    // The compiler feature alone is not enough: a reflection-enabled compiler
    // may be paired with a standard library that does not ship <meta> (e.g.
    // clang-p2996 with libstdc++).
    #if (                                                                      \
        (defined(__cpp_lib_reflection) && __cpp_lib_reflection >= 202506L) ||  \
        (__has_feature(reflection) && __cplusplus > 202302L)) &&               \
        __has_include(<meta>)
        #define ATOM_HAS_REFLECTION 1
    #else
        #define ATOM_HAS_REFLECTION 0
    #endif
#endif

#ifndef ATOM_ENABLE_EXCEPTIONS
    #if _MSC_VER
        #include <vcruntime.h> // for _HAS_EXCEPTIONS
    #endif
    #if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || (_HAS_EXCEPTIONS)
        #define ATOM_ENABLE_EXCEPTIONS 1
    #else
        #define ATOM_ENABLE_EXCEPTIONS 0
    #endif
#endif

#if ATOM_ENABLE_EXCEPTIONS
    #define ATOM_TRY try
    #define ATOM_CATCH(...) catch (__VA_ARGS__)
    #define ATOM_RETHROW throw
#else
    #define ATOM_TRY
    #define ATOM_CATCH(...) if (false)
    #define ATOM_RETHROW
#endif

#ifndef ATOM_RESTRICT
    #if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
        #define ATOM_RESTRICT __restrict__
    #elif defined(_MSC_VER)
        #define ATOM_RESTRICT __restrict
    #else
        #define ATOM_RESTRICT
    #endif
#endif

#ifndef ATOM_ASSUME
    #if ATOM_DEBUG_MODE
        #define ATOM_ASSUME(expr)
    #else
        #ifdef _MSC_VER
            #define ATOM_ASSUME(expr) __assume(expr)
        #elif defined(__clang__)
            #define ATOM_ASSUME(expr) __builtin_assume(expr)
        #elif defined(__GNUC__)
            #define ATOM_ASSUME(expr) __attribute__((assume(expr)))
        #else
            #define ATOM_ASSUME(expr)
        #endif
    #endif
#endif

#ifndef ATOM_ASSERT
    #if ATOM_DEBUG_MODE
        #include <cassert>
        #define ATOM_ASSERT(expr) assert(expr)
    #else
        #define ATOM_ASSERT(expr)
    #endif
#endif

#ifndef ATOM_ATTRS
// NOLINTNEXTLINE
    #define ATOM_ATTRS(...) [[__VA_ARGS__]]
#endif

#ifndef ATOM_ALWAYS_INLINE
    #ifdef _MSC_VER
        #define ATOM_ALWAYS_INLINE msvc::forceinline
    #elif defined(__clang__)
        #define ATOM_ALWAYS_INLINE clang::always_inline
    #elif defined(__GNUC__)
        #define ATOM_ALWAYS_INLINE gnu::always_inline
    #else
        #define ATOM_ALWAYS_INLINE
    #endif
#endif

#ifndef ATOM_NOINLINE
    #ifdef _MSC_VER
        #define ATOM_NOINLINE msvc::noinline
    #elif defined(__clang__)
        #define ATOM_NOINLINE clang::noinline
    #elif defined(__GNUC__)
        #define ATOM_NOINLINE gnu::noinline
    #else
        #define ATOM_NOINLINE
    #endif
#endif

#ifndef ATOM_NODEBUG
    #ifdef _MSC_VER
        #define ATOM_NODEBUG
    #elif defined(__clang__)
        #define ATOM_NODEBUG gnu::nodebug
    #elif defined(__GNUC__)
        #define ATOM_NODEBUG gnu::artificial
    #else
        #define ATOM_NODEBUG
    #endif
#endif
