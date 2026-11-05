#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include <neutron/args.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

using count_arg    = argument<int, "-n", "count of items">;
using name_arg     = argument<std::string, "--name", "user name">;
using files_arg    = multi_argument<std::string, "-f", "input files">;
using parser_t     = parser<count_arg, name_arg, files_arg>;
using reserved_arg = argument<int, "-", "conflicts with remains">;

/// Satisfied when `parser<Args...>` is a valid type.
template <typename... Args>
concept valid_parser = requires { typename parser<Args...>; };

/// Result type of `get<Alpha>()` on a const parser.
template <tstring Alpha>
using get_result =
    decltype(std::declval<const parser_t&>().template get<Alpha>());

// Static metadata
static_assert(count_arg::alpha() == "-n");
static_assert(count_arg::meaning() == "count of items");
static_assert(name_arg::alpha() == "--name");
static_assert(files_arg::alpha() == "-f");
static_assert(files_arg::meaning() == "input files");
static_assert(remain_arguments::alpha() == "-");
static_assert(remain_arguments::meaning() == "remains");
static_assert(noexcept(count_arg::alpha()));
static_assert(noexcept(count_arg::meaning()));
static_assert(std::is_same_v<decltype(count_arg::alpha()), std::string_view>);

// Value members
static_assert(std::is_same_v<decltype(count_arg::value), int>);
static_assert(
    std::is_same_v<decltype(files_arg::values), std::vector<std::string>>);
static_assert(
    std::is_same_v<decltype(remain_arguments::value), const char* const*>);
static_assert(std::is_aggregate_v<count_arg>);

constexpr bool argument_is_mutable_in_constexpr() {
    count_arg arg{ 5 };
    arg.value += 1;
    return arg.value == 6;
}
static_assert(argument_is_mutable_in_constexpr());

// The alpha `-` is reserved for the remaining arguments.
static_assert(valid_parser<count_arg, name_arg>);
static_assert(!valid_parser<count_arg, reserved_arg>);

// `get` finds arguments by their alpha.
static_assert(std::is_same_v<get_result<"-n">, const count_arg&>);
static_assert(std::is_same_v<get_result<"--name">, const name_arg&>);
static_assert(std::is_same_v<get_result<"-f">, const files_arg&>);
static_assert(std::is_same_v<get_result<"-">, const remain_arguments&>);
static_assert(noexcept(std::declval<const parser_t&>().get<"-n">()));

void test_single_argument_holds_its_value() {
    const std::string user(40, 'u');

    name_arg name{ std::string(user) };

    require(name.value == user);
}

void test_multi_argument_collects_values() {
    constexpr std::size_t long_length = 64;
    files_arg files{
        { "a.txt", "b.txt" }
    };

    files.values.emplace_back(std::string(long_length, 'c'));

    require_or_return(files.values.size() == 3, );
    require(files.values.back().size() == long_length);
}

void test_parser_value_initializes_its_arguments() {
    const parser_t parser{};

    require(parser.get<"-n">().value == 0);
    require(parser.get<"--name">().value.empty());
    require(parser.get<"-f">().values.empty());
    require(parser.get<"-">().value == nullptr);
}

void test_parse_stops_at_program_name_and_separator() {
    parser_t parser{};
    const char* const only_program[] = { "program" };               // NOLINT
    const char* const separator[]    = { "program", "--", "rest" }; // NOLINT

    parser.parse(1, only_program);
    // `--` stops parsing immediately.
    parser.parse(3, separator);

    require(parser.get<"-n">().value == 0);
}

} // namespace

int main() {
    test_single_argument_holds_its_value();
    test_multi_argument_collects_values();
    test_parser_value_initializes_its_arguments();
    test_parse_stops_at_program_name_and_separator();
    return neutron_test::result();
}
