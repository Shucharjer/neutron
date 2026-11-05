#include <cstddef>
#include <string>
#include <type_traits>
#include <neutron/detail/member_function_traits.hpp>
#include "require.hpp"

using namespace neutron;

namespace {

/// Members with every combination of const, noexcept and void return.
struct widget {
    int value = 1;

    int add(int arg) { return value += arg; }
    int peek(int arg) const { return value + arg; }
    int add_noexcept(int arg) noexcept { return value += arg; }
    int peek_noexcept(int arg) const noexcept { return value + arg; }

    void set(int arg) { value = arg; }
    void set_noexcept(int arg) noexcept { value = arg; }
    void touch() const {}
    void touch_noexcept() const noexcept {}

    std::string describe(const std::string& prefix, int times) const {
        std::string result = prefix;
        for (int i = 0; i != times; ++i) {
            result += std::to_string(value);
        }
        return result;
    }
    void nullary() {}
};

/// Checks every member type of the traits of `Fn`, a member of `widget`.
template <
    auto Fn, typename Type, typename Function, typename Static, typename Void,
    typename Erased, typename Ret, typename... Args>
constexpr bool traits_are() {
    using traits = member_function_traits<decltype(Fn)>;
    static_assert(std::is_same_v<typename traits::type, Type>);
    static_assert(std::is_same_v<typename traits::function_type, Function>);
    static_assert(std::is_same_v<typename traits::static_type, Static>);
    static_assert(std::is_same_v<typename traits::void_type, Void>);
    static_assert(std::is_same_v<typename traits::erased_type, Erased>);
    static_assert(std::is_same_v<typename traits::return_type, Ret>);
    static_assert(std::is_same_v<typename traits::class_type, widget>);
    static_assert(
        std::is_same_v<typename traits::args_type, type_list<Args...>>);
    static_assert(traits::num_args == sizeof...(Args));
    return true;
}

/// Traits of the member function `Fn`.
template <auto Fn>
using traits_of = member_function_traits<decltype(Fn)>;

// Each block lists, in order: type, function_type, static_type, void_type,
// erased_type, return_type and the argument types.

// Non-const, non-noexcept
static_assert(
    traits_are<
        &widget::add, int (widget::*)(int), int(int), int (*)(widget&, int),
        int (*)(void*, int), int (*)(int), int, int>());
static_assert(!traits_of<&widget::add>::is_noexcept);

// Const
static_assert(traits_are<
              &widget::peek, int (widget::*)(int) const, int(int),
              int (*)(const widget&, int), int (*)(const void*, int),
              int (*)(int), int, int>());
static_assert(!traits_of<&widget::peek>::is_noexcept);

// Noexcept
static_assert(traits_are<
              &widget::add_noexcept, int (widget::*)(int) noexcept,
              int(int) noexcept, int (*)(widget&, int) noexcept,
              int (*)(void*, int) noexcept, int (*)(int) noexcept, int, int>());
static_assert(traits_of<&widget::add_noexcept>::is_noexcept);

// Const noexcept
static_assert(
    traits_are<
        &widget::peek_noexcept, int (widget::*)(int) const noexcept,
        int(int) noexcept, int (*)(const widget&, int) noexcept,
        int (*)(const void*, int) noexcept, int (*)(int) noexcept, int, int>());
static_assert(traits_of<&widget::peek_noexcept>::is_noexcept);

// Multiple arguments, references and void returns
static_assert(traits_are<
              &widget::describe,
              std::string (widget::*)(const std::string&, int) const,
              std::string(const std::string&, int),
              std::string (*)(const widget&, const std::string&, int),
              std::string (*)(const void*, const std::string&, int),
              std::string (*)(const std::string&, int), std::string,
              const std::string&, int>());
static_assert(traits_are<
              &widget::nullary, void (widget::*)(), void(), void (*)(widget&),
              void (*)(void*), void (*)(), void>());
static_assert(traits_of<&widget::nullary>::num_args == 0);

// The generated thunks convert to the advertised pointer types...
static_assert(std::is_convertible_v<
              decltype(member_fn<&widget::add>::void_fn),
              member_function_traits<decltype(&widget::add)>::void_type>);
static_assert(
    std::is_convertible_v<
        decltype(member_fn<&widget::peek_noexcept>::void_fn),
        member_function_traits<decltype(&widget::peek_noexcept)>::void_type>);
static_assert(std::is_convertible_v<
              decltype(member_fn<&widget::set>::static_fn),
              member_function_traits<decltype(&widget::set)>::static_type>);
static_assert(std::is_convertible_v<
              decltype(member_fn<&widget::touch_noexcept>::static_fn),
              member_function_traits<
                  decltype(&widget::touch_noexcept)>::static_type>);
// ...and are noexcept exactly when the member is.
static_assert(std::is_nothrow_invocable_v<
              decltype(member_fn<&widget::add_noexcept>::void_fn), void*, int>);
static_assert(!std::is_nothrow_invocable_v<
              decltype(member_fn<&widget::add>::void_fn), void*, int>);
static_assert(
    std::is_nothrow_invocable_v<
        decltype(member_fn<&widget::set_noexcept>::static_fn), widget&, int>);
static_assert(!std::is_nothrow_invocable_v<
              decltype(member_fn<&widget::set>::static_fn), widget&, int>);

void test_void_fn_calls_mutating_member() {
    widget object;
    void* erased                           = &object;
    traits_of<&widget::add>::void_type add = member_fn<&widget::add>::void_fn;

    const int result = add(erased, 2);

    require(result == 3);
    require(object.value == 3);
}

void test_void_fn_calls_const_member_without_mutating() {
    widget object{ 3 };
    const void* const_erased = &object;
    traits_of<&widget::peek>::void_type peek =
        member_fn<&widget::peek>::void_fn;

    const int result = peek(const_erased, 10);

    require(result == 13);
    require(object.value == 3);
}

void test_void_fn_calls_noexcept_members() {
    widget object{ 3 };
    void* erased             = &object;
    const void* const_erased = &object;
    traits_of<&widget::add_noexcept>::void_type add_noexcept =
        member_fn<&widget::add_noexcept>::void_fn;
    traits_of<&widget::peek_noexcept>::void_type peek_noexcept =
        member_fn<&widget::peek_noexcept>::void_fn;

    require(add_noexcept(erased, 1) == 4);
    require(peek_noexcept(const_erased, 1) == 5);
}

void test_void_fn_forwards_multiple_arguments() {
    widget object{ 4 };
    const void* const_erased = &object;

    const auto text =
        member_fn<&widget::describe>::void_fn(const_erased, "v", 2);

    require(text == "v44");
}

// Only void members: the static thunks do not forward a return value.
void test_static_fn_calls_void_members() {
    widget object;
    traits_of<&widget::set>::static_type set =
        member_fn<&widget::set>::static_fn;
    traits_of<&widget::set_noexcept>::static_type set_noexcept =
        member_fn<&widget::set_noexcept>::static_fn;
    traits_of<&widget::touch>::static_type touch =
        member_fn<&widget::touch>::static_fn;
    traits_of<&widget::touch_noexcept>::static_type touch_noexcept =
        member_fn<&widget::touch_noexcept>::static_fn;

    set(object, 8);
    require(object.value == 8);

    set_noexcept(object, 9);
    touch(object);
    touch_noexcept(object);
    member_fn<&widget::nullary>::static_fn(object);
    member_fn<&widget::nullary>::void_fn(&object);
    // The const and nullary members leave the value alone.
    require(object.value == 9);
}

} // namespace

int main() {
    test_void_fn_calls_mutating_member();
    test_void_fn_calls_const_member_without_mutating();
    test_void_fn_calls_noexcept_members();
    test_void_fn_forwards_multiple_arguments();
    test_static_fn_calls_void_members();
    return neutron_test::result();
}
