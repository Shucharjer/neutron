#pragma once

namespace neutron::_reflection {

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define EXPAND(...) __VA_ARGS__

#define NUM_ARGS_HELPER_(                                                      \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16,     \
    _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, \
    _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, \
    _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, \
    _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, \
    _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104,      \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116,    \
    _117, _118, _119, _120, _121, _122, _123, _124, _125, N, ...)              \
    N

#define NUM_ARGS_HELPER(...)                                                   \
    EXPAND(NUM_ARGS_HELPER_(                                                   \
        __VA_ARGS__, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115,    \
        114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101,  \
        100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84,   \
        83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67,    \
        66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50,    \
        49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33,    \
        32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,    \
        15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1))

#define NUM_ARGS(...) NUM_ARGS_HELPER(__VA_ARGS__)

#define CONCAT_(l, r) l##r
#define CONCAT(l, r) CONCAT_(l, r)

#define FIELD_TRAITS(_, x)                                                     \
    ::neutron::field_traits<decltype(&_::x)> { #x, &_::x }                     \
    //

#define FUNCTION_TRAITS(_, x)                                                  \
    ::neutron::function_traits<decltype(&_::x)> { #x, &_::x }                  \
    //

#define FOR_EACH_0(MACRO, _)
#define FOR_EACH_1(MACRO, _, _1) MACRO(_, _1)
#define FOR_EACH_2(MACRO, _, _1, ...)                                          \
    MACRO(_, _1), EXPAND(FOR_EACH_1(MACRO, _, __VA_ARGS__))
#define FOR_EACH_3(MACRO, _, _1, ...)                                          \
    MACRO(_, _1), EXPAND(FOR_EACH_2(MACRO, _, __VA_ARGS__))
#define FOR_EACH_4(MACRO, _, _1, ...)                                          \
    MACRO(_, _1), EXPAND(FOR_EACH_3(MACRO, _, __VA_ARGS__))
#define FOR_EACH_5(MACRO, _, _1, ...)                                          \
    MACRO(_, _1), EXPAND(FOR_EACH_4(MACRO, _, __VA_ARGS__))
#define FOR_EACH_6(MACRO, _, _1, ...)                                          \
    MACRO(_, _1), EXPAND(FOR_EACH_5(MACRO, _, __VA_ARGS__))
#define FOR_EACH_7(MACRO, _, _1, ...)                                          \
    MACRO(_, _1), EXPAND(FOR_EACH_6(MACRO, _, __VA_ARGS__))
#define FOR_EACH_8(MACRO, _, _1, ...)                                          \
    MACRO(_, _1), EXPAND(FOR_EACH_7(MACRO, _, __VA_ARGS__))
#define FOR_EACH_9(MACRO, _, _1, ...)                                          \
    MACRO(_, _1), EXPAND(FOR_EACH_8(MACRO, _, __VA_ARGS__))
#define FOR_EACH_10(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_9(MACRO, _, __VA_ARGS__))
#define FOR_EACH_11(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_10(MACRO, _, __VA_ARGS__))
#define FOR_EACH_12(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_11(MACRO, _, __VA_ARGS__))
#define FOR_EACH_13(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_12(MACRO, _, __VA_ARGS__))
#define FOR_EACH_14(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_13(MACRO, _, __VA_ARGS__))
#define FOR_EACH_15(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_14(MACRO, _, __VA_ARGS__))
#define FOR_EACH_16(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_15(MACRO, _, __VA_ARGS__))
#define FOR_EACH_17(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_16(MACRO, _, __VA_ARGS__))
#define FOR_EACH_18(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_17(MACRO, _, __VA_ARGS__))
#define FOR_EACH_19(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_18(MACRO, _, __VA_ARGS__))
#define FOR_EACH_20(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_19(MACRO, _, __VA_ARGS__))
#define FOR_EACH_21(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_20(MACRO, _, __VA_ARGS__))
#define FOR_EACH_22(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_21(MACRO, _, __VA_ARGS__))
#define FOR_EACH_23(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_22(MACRO, _, __VA_ARGS__))
#define FOR_EACH_24(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_23(MACRO, _, __VA_ARGS__))
#define FOR_EACH_25(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_24(MACRO, _, __VA_ARGS__))
#define FOR_EACH_26(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_25(MACRO, _, __VA_ARGS__))
#define FOR_EACH_27(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_26(MACRO, _, __VA_ARGS__))
#define FOR_EACH_28(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_27(MACRO, _, __VA_ARGS__))
#define FOR_EACH_29(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_28(MACRO, _, __VA_ARGS__))
#define FOR_EACH_30(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_29(MACRO, _, __VA_ARGS__))
#define FOR_EACH_31(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_30(MACRO, _, __VA_ARGS__))
#define FOR_EACH_32(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_31(MACRO, _, __VA_ARGS__))
#define FOR_EACH_33(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_32(MACRO, _, __VA_ARGS__))
#define FOR_EACH_34(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_33(MACRO, _, __VA_ARGS__))
#define FOR_EACH_35(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_34(MACRO, _, __VA_ARGS__))
#define FOR_EACH_36(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_35(MACRO, _, __VA_ARGS__))
#define FOR_EACH_37(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_36(MACRO, _, __VA_ARGS__))
#define FOR_EACH_38(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_37(MACRO, _, __VA_ARGS__))
#define FOR_EACH_39(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_38(MACRO, _, __VA_ARGS__))
#define FOR_EACH_40(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_39(MACRO, _, __VA_ARGS__))
#define FOR_EACH_41(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_40(MACRO, _, __VA_ARGS__))
#define FOR_EACH_42(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_41(MACRO, _, __VA_ARGS__))
#define FOR_EACH_43(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_42(MACRO, _, __VA_ARGS__))
#define FOR_EACH_44(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_43(MACRO, _, __VA_ARGS__))
#define FOR_EACH_45(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_44(MACRO, _, __VA_ARGS__))
#define FOR_EACH_46(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_45(MACRO, _, __VA_ARGS__))
#define FOR_EACH_47(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_46(MACRO, _, __VA_ARGS__))
#define FOR_EACH_48(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_47(MACRO, _, __VA_ARGS__))
#define FOR_EACH_49(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_48(MACRO, _, __VA_ARGS__))
#define FOR_EACH_50(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_49(MACRO, _, __VA_ARGS__))
#define FOR_EACH_51(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_50(MACRO, _, __VA_ARGS__))
#define FOR_EACH_52(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_51(MACRO, _, __VA_ARGS__))
#define FOR_EACH_53(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_52(MACRO, _, __VA_ARGS__))
#define FOR_EACH_54(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_53(MACRO, _, __VA_ARGS__))
#define FOR_EACH_55(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_54(MACRO, _, __VA_ARGS__))
#define FOR_EACH_56(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_55(MACRO, _, __VA_ARGS__))
#define FOR_EACH_57(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_56(MACRO, _, __VA_ARGS__))
#define FOR_EACH_58(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_57(MACRO, _, __VA_ARGS__))
#define FOR_EACH_59(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_58(MACRO, _, __VA_ARGS__))
#define FOR_EACH_60(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_59(MACRO, _, __VA_ARGS__))
#define FOR_EACH_61(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_60(MACRO, _, __VA_ARGS__))
#define FOR_EACH_62(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_61(MACRO, _, __VA_ARGS__))
#define FOR_EACH_63(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_62(MACRO, _, __VA_ARGS__))
#define FOR_EACH_64(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_63(MACRO, _, __VA_ARGS__))
#define FOR_EACH_65(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_64(MACRO, _, __VA_ARGS__))
#define FOR_EACH_66(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_65(MACRO, _, __VA_ARGS__))
#define FOR_EACH_67(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_66(MACRO, _, __VA_ARGS__))
#define FOR_EACH_68(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_67(MACRO, _, __VA_ARGS__))
#define FOR_EACH_69(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_68(MACRO, _, __VA_ARGS__))
#define FOR_EACH_70(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_69(MACRO, _, __VA_ARGS__))
#define FOR_EACH_71(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_70(MACRO, _, __VA_ARGS__))
#define FOR_EACH_72(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_71(MACRO, _, __VA_ARGS__))
#define FOR_EACH_73(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_72(MACRO, _, __VA_ARGS__))
#define FOR_EACH_74(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_73(MACRO, _, __VA_ARGS__))
#define FOR_EACH_75(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_74(MACRO, _, __VA_ARGS__))
#define FOR_EACH_76(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_75(MACRO, _, __VA_ARGS__))
#define FOR_EACH_77(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_76(MACRO, _, __VA_ARGS__))
#define FOR_EACH_78(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_77(MACRO, _, __VA_ARGS__))
#define FOR_EACH_79(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_78(MACRO, _, __VA_ARGS__))
#define FOR_EACH_80(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_79(MACRO, _, __VA_ARGS__))
#define FOR_EACH_81(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_80(MACRO, _, __VA_ARGS__))
#define FOR_EACH_82(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_81(MACRO, _, __VA_ARGS__))
#define FOR_EACH_83(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_82(MACRO, _, __VA_ARGS__))
#define FOR_EACH_84(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_83(MACRO, _, __VA_ARGS__))
#define FOR_EACH_85(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_84(MACRO, _, __VA_ARGS__))
#define FOR_EACH_86(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_85(MACRO, _, __VA_ARGS__))
#define FOR_EACH_87(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_86(MACRO, _, __VA_ARGS__))
#define FOR_EACH_88(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_87(MACRO, _, __VA_ARGS__))
#define FOR_EACH_89(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_88(MACRO, _, __VA_ARGS__))
#define FOR_EACH_90(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_89(MACRO, _, __VA_ARGS__))
#define FOR_EACH_91(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_90(MACRO, _, __VA_ARGS__))
#define FOR_EACH_92(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_91(MACRO, _, __VA_ARGS__))
#define FOR_EACH_93(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_92(MACRO, _, __VA_ARGS__))
#define FOR_EACH_94(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_93(MACRO, _, __VA_ARGS__))
#define FOR_EACH_95(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_94(MACRO, _, __VA_ARGS__))
#define FOR_EACH_96(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_95(MACRO, _, __VA_ARGS__))
#define FOR_EACH_97(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_96(MACRO, _, __VA_ARGS__))
#define FOR_EACH_98(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_97(MACRO, _, __VA_ARGS__))
#define FOR_EACH_99(MACRO, _, _1, ...)                                         \
    MACRO(_, _1), EXPAND(FOR_EACH_98(MACRO, _, __VA_ARGS__))
#define FOR_EACH_100(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_99(MACRO, _, __VA_ARGS__))
#define FOR_EACH_101(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_100(MACRO, _, __VA_ARGS__))
#define FOR_EACH_102(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_101(MACRO, _, __VA_ARGS__))
#define FOR_EACH_103(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_102(MACRO, _, __VA_ARGS__))
#define FOR_EACH_104(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_103(MACRO, _, __VA_ARGS__))
#define FOR_EACH_105(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_104(MACRO, _, __VA_ARGS__))
#define FOR_EACH_106(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_105(MACRO, _, __VA_ARGS__))
#define FOR_EACH_107(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_106(MACRO, _, __VA_ARGS__))
#define FOR_EACH_108(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_107(MACRO, _, __VA_ARGS__))
#define FOR_EACH_109(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_108(MACRO, _, __VA_ARGS__))
#define FOR_EACH_110(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_109(MACRO, _, __VA_ARGS__))
#define FOR_EACH_111(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_110(MACRO, _, __VA_ARGS__))
#define FOR_EACH_112(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_111(MACRO, _, __VA_ARGS__))
#define FOR_EACH_113(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_112(MACRO, _, __VA_ARGS__))
#define FOR_EACH_114(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_113(MACRO, _, __VA_ARGS__))
#define FOR_EACH_115(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_114(MACRO, _, __VA_ARGS__))
#define FOR_EACH_116(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_115(MACRO, _, __VA_ARGS__))
#define FOR_EACH_117(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_116(MACRO, _, __VA_ARGS__))
#define FOR_EACH_118(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_117(MACRO, _, __VA_ARGS__))
#define FOR_EACH_119(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_118(MACRO, _, __VA_ARGS__))
#define FOR_EACH_120(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_119(MACRO, _, __VA_ARGS__))
#define FOR_EACH_121(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_120(MACRO, _, __VA_ARGS__))
#define FOR_EACH_122(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_121(MACRO, _, __VA_ARGS__))
#define FOR_EACH_123(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_122(MACRO, _, __VA_ARGS__))
#define FOR_EACH_124(MACRO, _, _1, ...)                                        \
    MACRO(_, _1), EXPAND(FOR_EACH_123(MACRO, _, __VA_ARGS__))

#define FOR_EACH_N(MACRO, _, ...)                                              \
    CONCAT(FOR_EACH_, NUM_ARGS(__VA_ARGS__))(MACRO, _, ##__VA_ARGS__)
#define FOR_EACH(MACRO, _, ...) FOR_EACH_N(MACRO, _, ##__VA_ARGS__)

#define REGISTER(type_name, register_name)                                     \
    namespace _internal::type_registers {                                      \
    static inline const ::neutron::type_register<type_name>(register_name);    \
    }                                                                          \
    //

// This macro expands specified structure, so you should make it in global
// namespace.
#define REFL_NAME(_, name)                                                     \
    template <>                                                                \
    struct ::neutron::nickname<_> {                                            \
        constexpr static inline std::string_view value = #name;                \
    }

// This macro expands function, so you should make it public.

// If you meet something wrong when using msvc, pleace try hover your cursor on
// the macro you wrote, and click expand inline
#define REFL_MEMBERS(_, ...)                                                   \
    constexpr static inline auto field_traits() noexcept {                     \
        return std::make_tuple(FOR_EACH(FIELD_TRAITS, _, ##__VA_ARGS__));      \
    } //

// This macro expands function, so you should make it public.

// If you meet something wrong when using msvc, pleace try hover your cursor on
// the macro you wrote, and click expand inline
#define REFL_FUNCS(_, ...)                                                     \
    constexpr static inline auto function_traits() noexcept {                  \
        return std::make_tuple(FOR_EACH(FUNCTION_TRAITS, _, ##__VA_ARGS__));   \
    } //

// NOLINTEND(cppcoreguidelines-macro-usage)

} // namespace neutron::_reflection
