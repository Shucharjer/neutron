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


#include <utility>


namespace neutron {




// in identity.hpp
class type_identity;
class non_type_identity;

/**
 * @class delegate
 * @brief A thin wrapper of function pointer and context.
 * It prefers performance over usability. We can only bind it by template
 * non-type arugment, which means it could be inlined by compiler as much as
 * possible.
 * @tparam Ret Return type of the function.
 * @tparam Args Arguments of the function.
 */
template <typename = void()>
class delegate;

template <typename = void>
class coroutine;

template <typename = void>
class lazy_routine;

template <typename Ty1, typename Ty2>
concept _not = (!std::same_as<Ty1, Ty2>);

template <typename Ty, typename... Args>
consteval auto make_array(Args&&... args) {
    return std::array<Ty, sizeof...(Args)>{ Ty{ std::forward<Args>(args) }... };
}

} // namespace neutron

using neutron::make_array;
