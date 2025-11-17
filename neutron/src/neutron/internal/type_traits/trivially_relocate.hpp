#pragma once

namespace neutron {

#if __has_builtin(__builtin_is_cpp_trivially_relocatable)
template <typename Ty>
concept trivially_relocatable = __builtin_is_cpp_trivially_relocatable(Ty);
#elif __has_builtin(__is_trivially_relocatable)
template <typename Ty>
concept bool trivially_relocatable = __is_trivially_relocatable(Ty);
#else
template <typename Ty>
concept trivially_relocatable = std::trivially_copyable_v<Ty>;
#endif

} // namespace neutron
