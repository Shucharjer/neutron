// IWYU pragma: private, include <neutron/reflection.hpp>
#pragma once
#include <array>
#include <concepts>
#include <cstddef>
#include <string_view>
#include <tuple>
#include <utility>
#include "neutron/detail/macros.hpp"

namespace neutron::_refl_legacy {

/**
 * @brief Extracts the spelling of a single template argument out of a function
 * signature.
 */
ATOM_NODISCARD consteval std::string_view extract_argument_name(
    std::string_view funcname, std::string_view marker) noexcept {
    const auto begin = funcname.find(marker);
    if (begin == std::string_view::npos) {
        return {};
    }

    const auto start = begin + marker.size();

#ifdef _MSC_VER
    const auto end = funcname.rfind(">(void)");
#elif defined(__clang__)
    const auto end = funcname.rfind(']');
#elif defined(__GNUC__)
    const auto alias = funcname.find(';', start);
    const auto end =
        alias == std::string_view::npos ? funcname.rfind(']') : alias;
#else
    const auto end = std::string_view::npos;
#endif

    if (end == std::string_view::npos || end < start) {
        return {};
    }

    return funcname.substr(start, end - start);
}

/**
 * @brief Returns the spelling the compiler gives to `Ty`.
 */
template <typename Ty>
ATOM_NODISCARD consteval std::string_view compiler_name_of() noexcept {
#ifdef _MSC_VER
    return extract_argument_name(__FUNCSIG__, "compiler_name_of<");
#else
    return extract_argument_name(__PRETTY_FUNCTION__, "Ty = ");
#endif
}

/**
 * @brief Returns the compiler spelling of a class template.
 */
template <template <typename...> class Tmpl>
ATOM_NODISCARD consteval std::string_view template_name_of() noexcept {
#ifdef _MSC_VER
    return extract_argument_name(__FUNCSIG__, "template_name_of<");
#else
    return extract_argument_name(__PRETTY_FUNCTION__, "Tmpl = ");
#endif
}

namespace _name_of {

/// @brief Recognizes the libstdc++ ABI and libc++ version namespaces.
ATOM_NODISCARD constexpr std::size_t
    inline_namespace_length(std::string_view raw, std::size_t pos) noexcept {
    constexpr std::string_view prefix = "std::__";

    if (!raw.substr(pos).starts_with(prefix)) {
        return 0;
    }

    const auto begin = pos + prefix.size();

    if (raw.substr(begin).starts_with("cxx11::")) {
        return prefix.size() + std::string_view{ "cxx11::" }.size();
    }

    std::size_t end = begin;
    while (end < raw.size() && raw[end] >= '0' && raw[end] <= '9') {
        ++end;
    }

    if (end == begin || !raw.substr(end).starts_with("::")) {
        return 0;
    }

    return end + 2 - pos;
}

/// @brief Measures a name, or writes it when a buffer is supplied.
struct name_sink {
    char* data       = nullptr;
    std::size_t size = 0;

    constexpr void append(char ch) noexcept {
        if (data != nullptr) {
            data[size] = ch;
        }
        ++size;
    }

    constexpr void append(std::string_view text) noexcept {
        for (const char ch : text) {
            append(ch);
        }
    }
};

/**
 * @brief Appends a type spelling with the compiler specific differences
 * removed.
 *
 * GCC and Clang disagree on the inline namespace the standard library lives in
 * (`std::__cxx11::basic_string` vs `std::basic_string`) and on the whitespace
 * of nested template arguments, pointers, references, function types and
 * arrays (`> >` vs `>>`, `int *` vs `int*`, `int* const` vs `int*const`,
 * `void (int)` vs `void(int)`, `int [4]` vs `int[4]`).
 *
 * @param sink The character sink to append to.
 * @param raw A type spelling as produced by `compiler_name_of`.
 */
constexpr void append_normalized(name_sink& sink, std::string_view raw) {
#ifdef _MSC_VER
    for (const std::string_view tag :
         { "class ", "struct ", "enum ", "union " }) {
        if (raw.starts_with(tag)) {
            raw.remove_prefix(tag.size());
        }
    }
#endif
    std::size_t pos = 0;
    char previous   = '\0';

    while (pos < raw.size()) {
        const auto skip = inline_namespace_length(raw, pos);
        if (skip != 0) {
            sink.append(std::string_view{ "std::" });
            previous = ':';
            pos += skip;
            continue;
        }

        const char ch = raw[pos];

        if (ch == ' ' && previous != '\0') {
            const char next  = pos + 1 < raw.size() ? raw[pos + 1] : '\0';
            const bool tight = (previous == '>' && next == '>') ||
                               next == '*' || next == '&' || next == '(' ||
                               next == '[' || previous == '*' ||
                               previous == '&';
            if (tight) {
                ++pos;
                continue;
            }
        }

        sink.append(ch);
        previous = ch;
        ++pos;
    }
}

/**
 * @brief Appends the canonical name of a type.
 *
 * A class template specialization is rebuilt from its template name and its
 * arguments, with the trailing arguments that only repeat the defaults
 * dropped. Every other type falls back to the compiler spelling.
 */
template <typename Ty>
struct name_writer {
    static constexpr void write(name_sink& sink) {
        append_normalized(sink, compiler_name_of<Ty>());
    }
};

template <template <typename...> class Tmpl, typename... Args>
struct name_writer<Tmpl<Args...>> {
    static constexpr void write(name_sink& sink) {
        constexpr auto tmpl_name = template_name_of<Tmpl>();
        if constexpr (tmpl_name.empty()) {
            append_normalized(sink, compiler_name_of<Tmpl<Args...>>());
        } else {
            append_normalized(sink, tmpl_name);
            sink.append('<');
            write_arguments(sink, std::make_index_sequence<min_prefix()>{});
            sink.append('>');
        }
    }

private:
    using _args_t = std::tuple<Args...>;

    /// @brief Omits arguments only if the prefix denotes the same type.
    template <std::size_t... Indices>
    static consteval bool prefix_is_same(std::index_sequence<Indices...>) {
        return requires {
            typename Tmpl<std::tuple_element_t<Indices, _args_t>...>;
            requires std::same_as<
                Tmpl<std::tuple_element_t<Indices, _args_t>...>, Tmpl<Args...>>;
        };
    }

    template <std::size_t Count = 0>
    static consteval std::size_t min_prefix() {
        if constexpr (
            Count == sizeof...(Args) ||
            prefix_is_same(std::make_index_sequence<Count>{})) {
            return Count;
        } else {
            return min_prefix<Count + 1>();
        }
    }

    template <std::size_t... Indices>
    static constexpr void
        write_arguments(name_sink& sink, std::index_sequence<Indices...>) {
        ((sink.append(Indices == 0 ? "" : ", "),
          name_writer<std::tuple_element_t<Indices, _args_t>>::write(sink)),
         ...);
    }
};

} // namespace _name_of

/**
 * @brief Fills the storage backing `name_of`.
 *
 * A `std::string_view` may only be handed out over an object with static
 * storage duration, hence the copy into an array. The trailing `'\0'` keeps
 * the name usable as a C string, like the compiler generated spellings are.
 */
template <typename Ty>
ATOM_NODISCARD constexpr auto make_canonical_name_storage() noexcept {
    constexpr auto size = [] {
        _name_of::name_sink sink;
        _name_of::name_writer<Ty>::write(sink);
        return sink.size;
    }();

    std::array<char, size + 1> storage{};
    _name_of::name_sink sink{ storage.data() };
    _name_of::name_writer<Ty>::write(sink);
    return storage;
}

/// @brief The compile time storage backing `name_of`.
template <typename Ty>
inline constexpr auto canonical_name_storage =
    make_canonical_name_storage<Ty>();

/**
 * @brief Customization point for legacy type names.
 *
 * Specialize it to give a type a hand written name.
 */
template <typename Ty>
struct name_of_type {
    static constexpr std::string_view value{
        canonical_name_storage<Ty>.data(), canonical_name_storage<Ty>.size() - 1
    };
};

/**
 * @brief Get the name of a type.
 *
 * Type-only templates omit trailing default arguments. Other types use the
 * normalized compiler spelling, which may still be compiler-specific.
 */
template <typename Ty>
ATOM_NODISCARD consteval std::string_view name_of() noexcept {
    return name_of_type<Ty>::value;
}

} // namespace neutron::_refl_legacy
