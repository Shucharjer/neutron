#pragma once
#include <cstddef>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#include "neutron/tstring.hpp"

namespace neutron {

template <typename Ty, tstring Alpha, tstring Meaning>
struct argument {
    constexpr static auto alpha() noexcept -> std::string_view {
        return Alpha.view();
    }
    constexpr static auto meaning() noexcept -> std::string_view {
        return Meaning.view();
    }
    Ty value;
};

template <typename Ty, tstring Alpha, tstring Meaning>
struct multi_argument {
    constexpr static auto alpha() noexcept -> std::string_view {
        return Alpha.view();
    }
    constexpr static auto meaning() noexcept -> std::string_view {
        return Meaning.view();
    }
    std::vector<Ty> values;
};

using remain_arguments = argument<const char* const*, "-", "remains">;

template <typename... Args>
requires((Args::alpha() != "-") && ...)
class parser {
    using tuple_type                   = std::tuple<Args..., remain_arguments>;
    constexpr static size_t args_count = sizeof...(Args) + 1;
    tuple_type args_;

    template <tstring Alpha>
    consteval static size_t _index() noexcept {
        size_t result = args_count;
        [&]<size_t... Is>(std::index_sequence<Is...>) {
            (..., (result = std::tuple_element_t<Is, tuple_type>::alpha() ==
                                    Alpha.view()
                                ? Is
                                : result));
        }(std::make_index_sequence<args_count>());
        return result;
    }

    void _try_match(std::string_view key, int& idx) {}

public:
    void parse(int argc, const char* const* argv) {
        auto idx = 1;                           // move forward by try_match
        while (idx < argc) {
            std::string_view view{ argv[idx] }; // NOLINT: c interface
            if (view[0] == '-') {
                if (view.length() == 2 && view[1] == '-') {
                    // remain arguments
                    remain_arguments& args = std::get<sizeof...(Args)>(args_);
                    // ...
                    return;
                }
                // try match
                // ...
            }
        }
    }
    template <tstring Alpha>
    ATOM_NODISCARD const auto& get() const noexcept {
        constexpr auto index = _index<Alpha>();
        static_assert(index != args_count, "No such argument");
        return std::get<index>(args_);
    }
};

} // namespace neutron
