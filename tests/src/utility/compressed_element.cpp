#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <neutron/utility.hpp>

using namespace neutron;

namespace {

struct empty {};

/// Tracks live objects to catch leaks and double destruction.
struct counted {
    static inline int live   = 0;
    static inline int copies = 0;
    static inline int moves  = 0;

    static void reset() noexcept {
        live   = 0;
        copies = 0;
        moves  = 0;
    }

    int value = 0;

    counted() noexcept { ++live; }
    counted(int val) noexcept : value(val) { ++live; }
    counted(const counted& that) noexcept : value(that.value) {
        ++live;
        ++copies;
    }
    counted(counted&& that) noexcept : value(that.value) {
        that.value = -1;
        ++live;
        ++moves;
    }
    counted& operator=(const counted& that) noexcept {
        value = that.value;
        ++copies;
        return *this;
    }
    counted& operator=(counted&& that) noexcept {
        value      = that.value;
        that.value = -1;
        ++moves;
        return *this;
    }
    ~counted() noexcept { --live; }
};

/// Every special member may throw.
struct throwing {
    throwing() noexcept(false) {}
    throwing(int) noexcept(false) {}
    throwing(const throwing&) noexcept(false) {}
    throwing(throwing&&) noexcept(false) {}
    throwing& operator=(const throwing&) noexcept(false) { return *this; }
    throwing& operator=(throwing&&) noexcept(false) { return *this; }
};

struct no_default {
    constexpr no_default(int val) noexcept : value(val) {}
    int value;
};

struct base {
    int value = 0;
};
struct derived : base {};

int add_one(int val) noexcept { return val + 1; }
int add_two(int val) noexcept { return val + 2; }

using int_elem  = _compressed_element<int, 0>;
using str_elem  = _compressed_element<std::string, 0>;
using ptr_elem  = _compressed_element<int*, 0>;
using fn_elem   = _compressed_element<int (*)(int), 0>;
using void_elem = _compressed_element<void, 0>;

// Member types
static_assert(std::is_same_v<int_elem::value_type, int>);
static_assert(std::is_same_v<int_elem::reference, int&>);
static_assert(std::is_same_v<int_elem::const_reference, const int&>);
static_assert(std::is_same_v<ptr_elem::value_type, int*>);
static_assert(std::is_same_v<ptr_elem::reference, int*&>);
static_assert(std::is_same_v<fn_elem::value_type, int (*)(int)>);
static_assert(std::is_same_v<fn_elem::reference, int (*&)(int)>);
static_assert(std::is_same_v<fn_elem::const_reference, int (*const&)(int)>);
static_assert(std::is_same_v<void_elem::value_type, void>);

// Layout
static_assert(sizeof(int_elem) == sizeof(int));
static_assert(sizeof(ptr_elem) == sizeof(int*));
static_assert(sizeof(fn_elem) == sizeof(int (*)(int)));
static_assert(std::is_empty_v<void_elem>);
static_assert(std::is_empty_v<_compressed_element<void, 1>>);

// Accessor value categories
static_assert(
    std::is_same_v<decltype(std::declval<str_elem&>().value()), std::string&>);
static_assert(
    std::is_same_v<
        decltype(std::declval<const str_elem&>().value()), const std::string&>);
static_assert(std::is_same_v<
              decltype(std::declval<str_elem&&>().value()), std::string&&>);
static_assert(
    std::is_same_v<decltype(std::declval<ptr_elem&>().value()), int*&>);
static_assert(std::is_same_v<
              decltype(std::declval<const ptr_elem&>().value()), int* const&>);
static_assert(
    std::is_same_v<decltype(std::declval<ptr_elem&&>().value()), int*>);
static_assert(
    std::is_same_v<decltype(std::declval<fn_elem&>().value()), int (*&)(int)>);
static_assert(
    std::is_same_v<
        decltype(std::declval<const fn_elem&>().value()), int (*const&)(int)>);
static_assert(
    std::is_same_v<decltype(std::declval<fn_elem&&>().value()), int (*)(int)>);
static_assert(
    std::is_same_v<decltype(std::declval<void_elem&>().value()), void>);
static_assert(noexcept(std::declval<str_elem&>().value()));
static_assert(noexcept(std::declval<const str_elem&>().value()));
static_assert(noexcept(std::declval<str_elem&&>().value()));
static_assert(noexcept(std::declval<const ptr_elem&>().value()));
static_assert(noexcept(std::declval<void_elem&>().value()));

// Construction constraints
static_assert(std::is_default_constructible_v<int_elem>);
static_assert(
    !std::is_default_constructible_v<_compressed_element<no_default, 0>>);
static_assert(std::is_constructible_v<_compressed_element<no_default, 0>, int>);
static_assert(std::is_constructible_v<str_elem, const char*>);
static_assert(std::is_constructible_v<str_elem, std::size_t, char>);
static_assert(!std::is_constructible_v<str_elem, int*>);
static_assert(!std::is_convertible_v<const char*, str_elem>);
static_assert(!std::is_convertible_v<int*, ptr_elem>);
static_assert(!std::is_convertible_v<int (*)(int), fn_elem>);
static_assert(std::is_constructible_v<ptr_elem, std::nullptr_t>);
static_assert(std::is_constructible_v<_compressed_element<base*, 0>, derived*>);
static_assert(std::is_constructible_v<fn_elem, std::nullptr_t>);

// Copy/move availability
static_assert(std::is_copy_constructible_v<str_elem>);
static_assert(std::is_copy_assignable_v<str_elem>);
static_assert(!std::is_copy_constructible_v<
              _compressed_element<std::unique_ptr<int>, 0>>);
static_assert(
    !std::is_copy_assignable_v<_compressed_element<std::unique_ptr<int>, 0>>);
static_assert(
    std::is_move_constructible_v<_compressed_element<std::unique_ptr<int>, 0>>);
static_assert(
    std::is_move_assignable_v<_compressed_element<std::unique_ptr<int>, 0>>);
static_assert(std::is_trivially_copy_constructible_v<int_elem>);
static_assert(std::is_trivially_copy_assignable_v<int_elem>);
static_assert(std::is_trivially_destructible_v<int_elem>);
static_assert(std::is_trivially_copy_constructible_v<ptr_elem>);
static_assert(std::is_trivially_copyable_v<fn_elem>);
static_assert(std::is_trivially_copyable_v<void_elem>);
static_assert(!std::is_trivially_destructible_v<str_elem>);

// noexcept propagation
static_assert(std::is_nothrow_default_constructible_v<int_elem>);
static_assert(std::is_nothrow_default_constructible_v<str_elem>);
static_assert(
    !std::is_nothrow_default_constructible_v<_compressed_element<throwing, 0>>);
static_assert(std::is_nothrow_constructible_v<int_elem, int>);
static_assert(!std::is_nothrow_constructible_v<str_elem, const char*>);
static_assert(
    !std::is_nothrow_constructible_v<_compressed_element<throwing, 0>, int>);
static_assert(std::is_nothrow_copy_constructible_v<int_elem>);
static_assert(!std::is_nothrow_copy_constructible_v<str_elem>);
static_assert(
    !std::is_nothrow_copy_constructible_v<_compressed_element<throwing, 0>>);
static_assert(std::is_nothrow_move_constructible_v<str_elem>);
static_assert(
    !std::is_nothrow_move_constructible_v<_compressed_element<throwing, 0>>);
static_assert(
    !std::is_nothrow_copy_assignable_v<_compressed_element<throwing, 0>>);
static_assert(
    !std::is_nothrow_move_assignable_v<_compressed_element<throwing, 0>>);
static_assert(std::is_nothrow_move_assignable_v<str_elem>);
static_assert(std::is_nothrow_copy_constructible_v<ptr_elem>);
static_assert(std::is_nothrow_move_constructible_v<ptr_elem>);
static_assert(std::is_nothrow_move_assignable_v<ptr_elem>);
static_assert(std::is_nothrow_move_assignable_v<fn_elem>);
static_assert(std::is_nothrow_move_assignable_v<void_elem>);
static_assert(std::is_nothrow_destructible_v<str_elem>);

constexpr bool test_constexpr() {
    int_elem def{};
    int_elem val{ 1 };
    int_elem copied{ val };
    int_elem moved{ std::move(copied) };
    def    = val;
    copied = std::move(moved);
    if (def.value() != 1 || copied.value() != 1 ||
        std::move(val).value() != 1) {
        return false;
    }
    val.value()          = 2;
    const int_elem& cref = val;
    if (cref.value() != 2) {
        return false;
    }

    int storage[2] = { 3, 4 };
    ptr_elem ptr{ storage };
    ptr_elem null_ptr{ nullptr };
    ptr_elem def_ptr;
    if (*ptr.value() != 3 || null_ptr.value() != nullptr ||
        def_ptr.value() != nullptr) {
        return false;
    }
    ptr_elem ptr_moved{ std::move(ptr) };
    if (ptr.value() != nullptr || ptr_moved.value() != storage) {
        return false;
    }
    ptr = storage + 1;
    if (*ptr.value() != 4) {
        return false;
    }
    ptr = nullptr;
    ptr = std::move(ptr_moved);
    if (ptr.value() != storage || ptr_moved.value() != nullptr) {
        return false;
    }

    _compressed_element<no_default, 1> nd{ 5 };
    void_elem none;
    none.value();
    return nd.value().value == 5;
}

static_assert(test_constexpr());

bool test_generic() {
    str_elem def;
    str_elem val{ "abc" };
    str_elem fill{ std::size_t{ 3 }, 'x' };
    if (!def.value().empty() || val.value() != "abc" || fill.value() != "xxx") {
        return false;
    }

    str_elem copied{ val };
    if (copied.value() != "abc" || val.value() != "abc") {
        return false;
    }
    str_elem moved{ std::move(copied) };
    if (moved.value() != "abc") {
        return false;
    }

    def = val;
    if (def.value() != "abc") {
        return false;
    }
    def = str_elem{ "def" };
    if (def.value() != "def") {
        return false;
    }

    // Self move assignment keeps the value.
    auto& alias = def;
    def         = std::move(alias);
    if (def.value() != "def") {
        return false;
    }

    std::string taken = std::move(def).value();
    if (taken != "def") {
        return false;
    }

    _compressed_element<std::unique_ptr<int>, 0> uniq{ std::make_unique<int>(
        1) };
    _compressed_element<std::unique_ptr<int>, 0> uniq_moved{ std::move(uniq) };
    if (uniq.value() || !uniq_moved.value() || *uniq_moved.value() != 1) {
        return false;
    }
    uniq = std::move(uniq_moved);
    if (!uniq.value() || *uniq.value() != 1 || uniq_moved.value()) {
        return false;
    }

    _compressed_element<empty, 0> emp;
    _compressed_element<empty, 0> emp_copy{ emp };
    (void)emp_copy.value();
    return true;
}

bool test_lifetime() {
    counted::reset();
    {
        _compressed_element<counted, 0> def;
        _compressed_element<counted, 0> val{ 1 };
        _compressed_element<counted, 0> copied{ val };
        _compressed_element<counted, 0> moved{ std::move(copied) };
        if (counted::live != 4 || counted::copies != 1 || counted::moves != 1 ||
            moved.value().value != 1 || copied.value().value != -1) {
            return false;
        }
        def = val;
        val = std::move(moved);
        if (def.value().value != 1 || val.value().value != 1 ||
            counted::live != 4) {
            return false;
        }
    }
    return counted::live == 0;
}

bool test_pointer() {
    int value = 1;
    ptr_elem ptr{ &value };
    ptr_elem copied{ ptr };
    if (copied.value() != &value || ptr.value() != &value) {
        return false;
    }

    copied = nullptr;
    if (copied.value() != nullptr) {
        return false;
    }
    copied = ptr;
    if (copied.value() != &value) {
        return false;
    }

    // Moving leaves the source empty.
    ptr_elem moved{ std::move(ptr) };
    if (moved.value() != &value || ptr.value() != nullptr) {
        return false;
    }
    ptr = std::move(moved);
    if (ptr.value() != &value || moved.value() != nullptr) {
        return false;
    }

    *ptr.value()         = 2;
    const ptr_elem& cref = ptr;
    if (*cref.value() != 2 || std::move(ptr).value() != &value) {
        return false;
    }

    derived obj;
    obj.value = 3;
    _compressed_element<base*, 0> upcast{ &obj };
    if (upcast.value()->value != 3) {
        return false;
    }
    upcast = static_cast<derived*>(nullptr);
    if (upcast.value() != nullptr) {
        return false;
    }

    const int cvalue = 4;
    _compressed_element<const int*, 0> to_const{ &cvalue };
    return *to_const.value() == 4;
}

bool test_function_pointer() {
    fn_elem def;
    fn_elem null_fn{ nullptr };
    if (def.value() != nullptr || null_fn.value() != nullptr) {
        return false;
    }

    fn_elem one{ &add_one };
    fn_elem copied{ one };
    if (one.value()(1) != 2 || copied.value()(1) != 2) {
        return false;
    }

    fn_elem moved{ std::move(copied) };
    if (moved.value() != &add_one) {
        return false;
    }

    copied = fn_elem{ &add_two };
    if (copied.value()(1) != 3) {
        return false;
    }
    copied = nullptr;
    if (copied.value() != nullptr) {
        return false;
    }
    copied              = one;
    const fn_elem& cref = copied;
    return cref.value()(2) == 3 && std::move(copied).value() == &add_one;
}

bool test_index_distinguishes() {
    // Distinct indices allow a class to derive from two elements of one type.
    struct twin : _compressed_element<int, 0>, _compressed_element<int, 1> {
        twin(int lhs, int rhs)
            : _compressed_element<int, 0>(lhs),
              _compressed_element<int, 1>(rhs) {}
    };
    twin both{ 1, 2 };
    return static_cast<_compressed_element<int, 0>&>(both).value() == 1 &&
           static_cast<_compressed_element<int, 1>&>(both).value() == 2;
}

} // namespace

int main() {
    if (!test_generic()) {
        return 1;
    }
    if (!test_lifetime()) {
        return 2;
    }
    if (!test_pointer()) {
        return 3;
    }
    if (!test_function_pointer()) {
        return 4;
    }
    if (!test_index_distinguishes()) {
        return 5;
    }
    return 0;
}
