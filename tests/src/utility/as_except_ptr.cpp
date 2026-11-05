#include <cerrno>
#include <cstddef>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <neutron/utility.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// User-defined exception type that does not derive from std::exception.
struct custom_error {
    int code;
    std::string message;
};

/// Rethrows `ptr` and returns a copy of the exception if it is caught as `E`.
template <typename E>
std::optional<E> rethrown_as(const std::exception_ptr& ptr) {
    if (!ptr) {
        return std::nullopt;
    }
    ATOM_TRY { std::rethrow_exception(ptr); }
    ATOM_CATCH(const E& error) { return error; }
    ATOM_CATCH(...) { return std::nullopt; }
    return std::nullopt;
}

// An exception_ptr is forwarded as a reference, keeping its value category.
static_assert(std::is_same_v<
              decltype(as_except_ptr(std::declval<std::exception_ptr&>())),
              std::exception_ptr&>);
static_assert(
    std::is_same_v<
        decltype(as_except_ptr(std::declval<const std::exception_ptr&>())),
        const std::exception_ptr&>);
static_assert(std::is_same_v<
              decltype(as_except_ptr(std::declval<std::exception_ptr&&>())),
              std::exception_ptr&&>);

// Anything else is wrapped into a new exception_ptr.
static_assert(std::is_same_v<
              decltype(as_except_ptr(std::declval<const std::error_code&>())),
              std::exception_ptr>);
static_assert(std::is_same_v<
              decltype(as_except_ptr(std::declval<std::logic_error&>())),
              std::exception_ptr>);

void test_exception_ptr_lvalue_is_the_same_object() {
    auto origin = std::make_exception_ptr(std::runtime_error("origin"));

    require(&as_except_ptr(origin) == &origin);
}

void test_exception_ptr_survives_copy_and_move() {
    auto origin = std::make_exception_ptr(std::runtime_error("origin"));

    std::exception_ptr copy  = as_except_ptr(origin);
    std::exception_ptr moved = as_except_ptr(std::move(copy));

    require(moved == origin);
    auto caught = rethrown_as<std::runtime_error>(moved);
    require_or_return(caught.has_value(), );
    require(std::string(caught->what()) == "origin");
}

void test_error_code_lvalue_becomes_system_error() {
    const std::error_code code =
        std::make_error_code(std::errc::invalid_argument);

    std::exception_ptr ptr = as_except_ptr(code);

    auto caught = rethrown_as<std::system_error>(ptr);
    require_or_return(caught.has_value(), );
    require(caught->code() == code);
}

void test_error_code_rvalue_becomes_system_error() {
    std::exception_ptr ptr =
        as_except_ptr(std::error_code(EDOM, std::generic_category()));

    auto caught = rethrown_as<std::system_error>(ptr);
    require_or_return(caught.has_value(), );
    require(caught->code().value() == EDOM);
    require(caught->code().category() == std::generic_category());
}

void test_standard_exception_lvalue_is_rethrown() {
    std::logic_error logic("logic");

    auto ptr = as_except_ptr(logic);

    auto caught = rethrown_as<std::logic_error>(ptr);
    require_or_return(caught.has_value(), );
    require(std::string(caught->what()) == "logic");
}

void test_derived_exception_is_catchable_through_base() {
    auto ptr = as_except_ptr(std::out_of_range("range"));

    auto as_derived = rethrown_as<std::out_of_range>(ptr);
    require_or_return(as_derived.has_value(), );
    require(std::string(as_derived->what()) == "range");
    require(rethrown_as<std::logic_error>(ptr).has_value());
}

void test_custom_type_is_rethrown() {
    constexpr int code           = 7;
    constexpr std::size_t length = 48; // beyond the small-string buffer
    const std::string message(length, 'e');
    custom_error error{ code, message };

    auto from_lvalue = as_except_ptr(error);
    auto from_rvalue = as_except_ptr(std::move(error));

    auto copied = rethrown_as<custom_error>(from_lvalue);
    require_or_return(copied.has_value(), );
    require(copied->code == code);
    require(copied->message == message);

    auto moved = rethrown_as<custom_error>(from_rvalue);
    require_or_return(moved.has_value(), );
    require(moved->message == message);
}

void test_builtin_and_library_values_are_rethrown() {
    auto from_int    = as_except_ptr(42);
    auto from_string = as_except_ptr(std::string("text"));

    require(rethrown_as<int>(from_int) == 42);
    require(rethrown_as<std::string>(from_string) == "text");
}

} // namespace

int main() {
    test_exception_ptr_lvalue_is_the_same_object();
    test_exception_ptr_survives_copy_and_move();
    test_error_code_lvalue_becomes_system_error();
    test_error_code_rvalue_becomes_system_error();
    test_standard_exception_lvalue_is_rethrown();
    test_derived_exception_is_catchable_through_base();
    test_custom_type_is_rethrown();
    test_builtin_and_library_values_are_rethrown();
    return neutron_test::result();
}
