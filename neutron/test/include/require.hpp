#pragma once
#include <source_location>
#include <neutron/print.hpp>

template <typename Ty>
void require(
    const Ty&, const std::source_location& = std::source_location::current());

template <>
void require(const bool& expr, const std::source_location& loc) {
    if (!expr) {
        neutron::println(
            "expresstion in {} evaluated to false, {}: {}", loc.function_name(),
            loc.file_name(), loc.line());
    }
}

template <typename Ty>
void require(const Ty& expr, const std::source_location& loc) {
    require(static_cast<bool>(expr), loc);
}

template <typename Ty>
void require_false(
    const Ty&,
    const std::source_location& loc = std::source_location::current());

template <>
void require_false(const bool& expr, const std::source_location& loc) {
    if (expr) {
        neutron::println(
            "expression in {} evaluated to true, {}: {}", loc.function_name(),
            loc.file_name(), loc.line());
    }
}

template <typename Ty>
void require_false(const Ty& expr, const std::source_location& loc) {
    require_false(static_cast<bool>(expr), loc);
}

#define require_or_return(expr, ret)                                           \
    if (!static_cast<bool>((expr))) {                                                             \
        const auto& loc = std::source_location::current();                     \
        neutron::println(                                                      \
            "expression '{}' in {} evaluted to false, {}: {}", #expr,            \
            loc.function_name(), loc.file_name(), loc.line());                 \
        return ret;                                                            \
    }

#define require_false_or_return(expr, ret)                                     \
    if (static_cast<bool>((expr))) {                                                             \
        const auto& loc = std::source_location::current();                     \
        neutron::println(                                                      \
            "expression '{}' in {} evaluted to true, {}: {}", #expr,            \
            loc.function_name(), loc.file_name(), loc.line());                 \
        return ret;                                                            \
    }
