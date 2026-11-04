#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <neutron/utility.hpp>

using namespace neutron;

namespace {

struct empty {};

/// Tracks special member calls and live objects to catch leaks and extra
/// copies.
struct counted {
    static inline int live    = 0;
    static inline int copies  = 0;
    static inline int moves   = 0;
    static inline int destroy = 0;

    static void reset() noexcept {
        live    = 0;
        copies  = 0;
        moves   = 0;
        destroy = 0;
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
    ~counted() noexcept {
        --live;
        ++destroy;
    }
};

/// Neither copyable nor movable, constructible from two ints.
struct pinned {
    int a;
    int b;
    constexpr pinned(int lhs, int rhs) noexcept : a(lhs), b(rhs) {}
    pinned(const pinned&)            = delete;
    pinned& operator=(const pinned&) = delete;
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

/// Pair-like type exposing public data members.
struct public_like {
    using first_type  = int;
    using second_type = std::string;
    int first;
    std::string second;
};

/// Pair-like type exposing accessors.
class private_like {
public:
    using first_type  = int;
    using second_type = int;

    constexpr private_like(int first, int second) noexcept
        : first_(first), second_(second) {}

    ATOM_NODISCARD constexpr int first() const noexcept { return first_; }
    ATOM_NODISCARD constexpr int second() const noexcept { return second_; }

private:
    int first_;
    int second_;
};

static_assert(public_pair<public_like>);
static_assert(private_pair<private_like>);
static_assert(private_pair<compressed_pair<int, int>>);
static_assert(!public_pair<compressed_pair<int, int>>);

// Layout
static_assert(sizeof(compressed_pair<int, int>) == 2 * sizeof(int));
static_assert(
    sizeof(compressed_pair<char, int>) == sizeof(std::pair<char, int>));
static_assert(
    alignof(compressed_pair<char, double>) == alignof(std::pair<char, double>));

// Member types and tuple protocol
using ip = compressed_pair<int, std::string>;
static_assert(std::is_same_v<ip::first_type, int>);
static_assert(std::is_same_v<ip::second_type, std::string>);
static_assert(std::tuple_size_v<ip> == 2);
static_assert(std::tuple_size_v<const ip> == 2);
static_assert(std::is_same_v<std::tuple_element_t<0, ip>, int>);
static_assert(std::is_same_v<std::tuple_element_t<1, ip>, std::string>);
static_assert(
    std::is_same_v<std::tuple_element_t<1, const ip>, const std::string>);

// Accessor value categories
static_assert(std::is_same_v<decltype(std::declval<ip&>().first()), int&>);
static_assert(
    std::is_same_v<decltype(std::declval<const ip&>().first()), const int&>);
static_assert(std::is_same_v<decltype(std::declval<ip&&>().first()), int&&>);
static_assert(
    std::is_same_v<decltype(std::declval<const ip&&>().first()), const int&&>);
static_assert(
    std::is_same_v<decltype(std::declval<ip&>().second()), std::string&>);
static_assert(
    std::is_same_v<
        decltype(std::declval<const ip&>().second()), const std::string&>);
static_assert(
    std::is_same_v<decltype(std::declval<ip&&>().second()), std::string&&>);
static_assert(
    std::is_same_v<
        decltype(std::declval<const ip&&>().second()), const std::string&&>);
static_assert(std::is_same_v<decltype(std::declval<ip&>().get<0>()), int&>);
static_assert(
    std::is_same_v<decltype(std::declval<ip&>().get<1>()), std::string&>);
static_assert(
    std::is_same_v<
        decltype(std::declval<const ip&>().get<1>()), const std::string&>);
static_assert(noexcept(std::declval<ip&>().first()));
static_assert(noexcept(std::declval<const ip&>().second()));
static_assert(noexcept(std::declval<ip&>().get<0>()));

template <typename Ty, size_t Index>
concept has_get = requires(Ty& val) { val.template get<Index>(); };
static_assert(has_get<ip, 0>);
static_assert(has_get<ip, 1>);
static_assert(!has_get<ip, 2>);

// Deduction
static_assert(
    std::is_same_v<
        decltype(compressed_pair{ 1, 2.0 }), compressed_pair<int, double>>);
static_assert(std::is_same_v<
              decltype(make_compressed_pair(1, "abc")),
              compressed_pair<int, const char*>>);

// Copy/move availability
static_assert(std::is_copy_constructible_v<ip>);
static_assert(std::is_copy_assignable_v<ip>);
static_assert(
    !std::is_copy_constructible_v<compressed_pair<std::unique_ptr<int>, int>>);
static_assert(
    !std::is_copy_assignable_v<compressed_pair<std::unique_ptr<int>, int>>);
static_assert(std::is_nothrow_move_constructible_v<
              compressed_pair<std::unique_ptr<int>, int>>);
static_assert(std::is_nothrow_move_assignable_v<
              compressed_pair<std::unique_ptr<int>, int>>);
static_assert(
    std::is_trivially_copy_constructible_v<compressed_pair<int, int>>);
static_assert(std::is_trivially_destructible_v<compressed_pair<int, int>>);
static_assert(!std::is_trivially_destructible_v<ip>);

// noexcept propagation
static_assert(
    std::is_nothrow_default_constructible_v<compressed_pair<int, int>>);
static_assert(std::is_nothrow_default_constructible_v<ip>);
static_assert(
    !std::is_nothrow_default_constructible_v<compressed_pair<throwing, int>>);
static_assert(
    !std::is_nothrow_default_constructible_v<compressed_pair<int, throwing>>);
static_assert(
    std::is_nothrow_constructible_v<compressed_pair<int, int>, int, int>);
static_assert(!std::is_nothrow_constructible_v<ip, int, const char*>);
static_assert(
    !std::is_nothrow_constructible_v<compressed_pair<int, throwing>, int, int>);
static_assert(std::is_nothrow_copy_constructible_v<compressed_pair<int, int>>);
static_assert(!std::is_nothrow_copy_constructible_v<ip>);
static_assert(std::is_nothrow_move_constructible_v<ip>);
static_assert(
    !std::is_nothrow_move_constructible_v<compressed_pair<throwing, int>>);
static_assert(
    !std::is_nothrow_move_assignable_v<compressed_pair<int, throwing>>);
static_assert(
    !std::is_nothrow_copy_assignable_v<compressed_pair<throwing, int>>);
static_assert(std::is_nothrow_constructible_v<
              compressed_pair<int, int>, std::piecewise_construct_t,
              std::tuple<int>, std::tuple<int>>);
static_assert(!std::is_nothrow_constructible_v<
              compressed_pair<throwing, int>, std::piecewise_construct_t,
              std::tuple<int>, std::tuple<int>>);
static_assert(std::is_nothrow_constructible_v<ip, std::pair<int, std::string>>);
static_assert(std::is_nothrow_constructible_v<
              compressed_pair<long, long>, const compressed_pair<int, int>&>);
static_assert(std::is_nothrow_constructible_v<
              compressed_pair<long, long>, compressed_pair<int, int>&&>);
static_assert(!std::is_nothrow_constructible_v<
              ip, const compressed_pair<int, const char*>&>);
static_assert(noexcept(make_compressed_pair(1, 2)));
static_assert(
    !noexcept(make_compressed_pair(1, std::declval<const std::string&>())));

// Implicit conversions
static_assert(
    std::is_convertible_v<std::pair<int, int>, compressed_pair<int, int>>);
static_assert(std::is_convertible_v<
              compressed_pair<int, int>, compressed_pair<long, long>>);
static_assert(std::is_convertible_v<const ip&, std::pair<int, std::string>>);
static_assert(std::is_convertible_v<ip&&, std::pair<int, std::string>>);

constexpr bool test_constexpr() {
    compressed_pair<int, int> def;
    if (def.first() != 0 || def.second() != 0) {
        return false;
    }

    compressed_pair<int, int> val{ 1, 2 };
    compressed_pair<int, int> braced = { 3, 4 };
    compressed_pair<pinned, int> piece(
        std::piecewise_construct, std::forward_as_tuple(5, 6),
        std::forward_as_tuple(7));
    if (val.first() != 1 || braced.second() != 4 || piece.first().a != 5 ||
        piece.first().b != 6 || piece.second() != 7) {
        return false;
    }

    std::pair<int, int> spair{ 8, 9 };
    compressed_pair<int, int> from_lvalue{ std::move(spair) };
    compressed_pair<int, int> from_const{
        static_cast<const std::pair<int, int>&&>(std::pair<int, int>{ 10, 11 })
    };
    compressed_pair<int, int> from_rvalue{
        std::pair<int, int>{ 12, 13 }
    };
    compressed_pair<long, long> from_private{
        private_like{ 14, 15 }
    };
    compressed_pair<long, long> from_other{ val };
    if (from_lvalue.first() != 8 || from_lvalue.second() != 9 ||
        from_const.first() != 10 || from_const.second() != 11 ||
        from_rvalue.first() != 12 || from_rvalue.second() != 13 ||
        from_private.first() != 14 || from_private.second() != 15 ||
        from_other.first() != 1 || from_other.second() != 2) {
        return false;
    }

    compressed_pair<int, int> copied{ val };
    compressed_pair<int, int> moved{ std::move(copied) };
    moved                 = braced;
    copied                = std::move(from_lvalue);
    auto made             = make_compressed_pair(16, 17);
    auto& [first, second] = made;
    first                 = 18;
    if (moved.get<0>() != 3 || copied.get<1>() != 9 || made.first() != 18 ||
        second != 17) {
        return false;
    }

    const std::pair<int, int> back = made;
    return made == compressed_pair<int, int>{ 18, 17 } &&
           made != compressed_pair<int, int>{ 18, 0 } &&
           made == std::pair<int, int>{ 18, 17 } && back.first == 18 &&
           back.second == 17;
}

static_assert(test_constexpr());

bool test_default_construct() {
    compressed_pair<int, std::string> def;
    compressed_pair<int*, double> ptr;
    compressed_pair<empty, int> emp;
    counted::reset();
    {
        compressed_pair<counted, counted> cnt;
        if (counted::live != 2 || cnt.first().value != 0) {
            return false;
        }
    }
    return def.first() == 0 && def.second().empty() && ptr.first() == nullptr &&
           ptr.second() == 0.0 && emp.second() == 0 && counted::live == 0;
}

bool test_value_construct() {
    compressed_pair<int, std::string> pair{ 1, "abc" };
    if (pair.first() != 1 || pair.second() != "abc") {
        return false;
    }

    compressed_pair<std::unique_ptr<int>, int> uniq{ std::make_unique<int>(2),
                                                     3 };
    if (!uniq.first() || *uniq.first() != 2 || uniq.second() != 3) {
        return false;
    }

    auto made = make_compressed_pair(std::string("x"), "y");
    static_assert(std::is_same_v<
                  decltype(made), compressed_pair<std::string, const char*>>);
    if (made.first() != "x" || std::string(made.second()) != "y") {
        return false;
    }

    counted::reset();
    {
        counted lhs{ 4 };
        counted rhs{ 5 };
        compressed_pair<counted, counted> cnt{ lhs, std::move(rhs) };
        if (counted::copies != 1 || counted::moves != 1 ||
            cnt.first().value != 4 || cnt.second().value != 5) {
            return false;
        }
    }
    return counted::live == 0;
}

bool test_piecewise() {
    compressed_pair<pinned, std::string> pair(
        std::piecewise_construct, std::forward_as_tuple(1, 2),
        std::forward_as_tuple(3, 'z'));
    if (pair.first().a != 1 || pair.first().b != 2 || pair.second() != "zzz") {
        return false;
    }

    compressed_pair<int, std::string> empty_args(
        std::piecewise_construct, std::tuple<>{}, std::tuple<>{});
    if (empty_args.first() != 0 || !empty_args.second().empty()) {
        return false;
    }

    // Arguments are forwarded directly into the elements.
    counted::reset();
    {
        counted source{ 6 };
        compressed_pair<counted, counted> cnt(
            std::piecewise_construct, std::forward_as_tuple(source),
            std::forward_as_tuple(7));
        if (counted::copies != 1 || counted::moves != 0 ||
            cnt.first().value != 6 || cnt.second().value != 7) {
            return false;
        }
    }
    return counted::live == 0;
}

bool test_from_std_pair() {
    const std::pair<int, std::string> clvalue{ 2, "two" };

    compressed_pair<int, std::string> from_const{ std::move(clvalue) };
    compressed_pair<int, std::string> from_rvalue{
        std::pair<int, std::string>{ 3, "three" }
    };
    compressed_pair<int, std::string> implicit =
        std::pair<int, std::string>{ 1, "one" };
    // A const rvalue source must be copied from, not moved from.
    if (from_const.first() != 2 || from_const.second() != "two" ||
        clvalue.second != "two" || from_rvalue.first() != 3 ||
        from_rvalue.second() != "three" || implicit.second() != "one") {
        return false;
    }

    // Heterogeneous element types.
    compressed_pair<std::string, long> converted{
        std::pair<const char*, int>{ "four", 4 }
    };
    if (converted.first() != "four" || converted.second() != 4) {
        return false;
    }

    // Move-only elements are moved out of an rvalue pair.
    std::pair<std::unique_ptr<int>, int> uniq{ std::make_unique<int>(5), 6 };
    compressed_pair<std::unique_ptr<int>, int> moved{ std::move(uniq) };
    if (!moved.first() || *moved.first() != 5 || moved.second() != 6 ||
        uniq.first) {
        return false;
    }

    // Rvalue sources are never copied.
    counted::reset();
    {
        std::pair<counted, counted> source{ 7, 8 };
        compressed_pair<counted, counted> from_move{ std::move(source) };
        if (counted::copies != 0 || from_move.first().value != 7 ||
            from_move.second().value != 8) {
            return false;
        }
    }
    return counted::live == 0;
}

bool test_from_pair_like() {
    public_like pub{ 1, "pub" };
    compressed_pair<int, std::string> from_pub{ std::move(pub) };
    compressed_pair<int, std::string> from_pub_rvalue{
        public_like{ 2, "rv" }
    };
    if (from_pub.first() != 1 || from_pub.second() != "pub" ||
        from_pub_rvalue.first() != 2 || from_pub_rvalue.second() != "rv") {
        return false;
    }

    compressed_pair<long, long> from_pvt{
        private_like{ 3, 4 }
    };
    if (from_pvt.first() != 3 || from_pvt.second() != 4) {
        return false;
    }

    // From another compressed_pair specialization.
    compressed_pair<int, const char*> narrow{ 5, "five" };
    const compressed_pair<int, const char*> cnarrow{ 6, "six" };
    compressed_pair<long, std::string> from_lvalue{ narrow };
    compressed_pair<long, std::string> from_const{ cnarrow };
    compressed_pair<long, std::string> from_rvalue{
        compressed_pair<int, std::string>{ 7, "seven" }
    };
    if (from_lvalue.first() != 5 || from_lvalue.second() != "five" ||
        from_const.first() != 6 || from_const.second() != "six" ||
        from_rvalue.first() != 7 || from_rvalue.second() != "seven") {
        return false;
    }

    compressed_pair<std::unique_ptr<const int>, int> uniq{
        compressed_pair<std::unique_ptr<int>, int>{ std::make_unique<int>(8),
                                                   9 }
    };
    return uniq.first() && *uniq.first() == 8 && uniq.second() == 9;
}

bool test_copy_move() {
    compressed_pair<int, std::string> origin{ 1, "abc" };
    compressed_pair<int, std::string> copied{ origin };
    if (copied.first() != 1 || copied.second() != "abc" ||
        origin.second() != "abc") {
        return false;
    }

    compressed_pair<int, std::string> moved{ std::move(copied) };
    if (moved.first() != 1 || moved.second() != "abc") {
        return false;
    }

    compressed_pair<int, std::string> assigned;
    assigned = origin;
    if (assigned.first() != 1 || assigned.second() != "abc") {
        return false;
    }
    assigned = compressed_pair<int, std::string>{ 2, "def" };
    if (assigned.first() != 2 || assigned.second() != "def") {
        return false;
    }

    // Self assignment keeps the value.
    auto& alias = assigned;
    assigned    = alias;
    assigned    = std::move(alias);
    if (assigned.first() != 2 || assigned.second() != "def") {
        return false;
    }

    compressed_pair<std::unique_ptr<int>, int> uniq{ std::make_unique<int>(3),
                                                     4 };
    compressed_pair<std::unique_ptr<int>, int> uniq_moved{ std::move(uniq) };
    if (uniq.first() || !uniq_moved.first() || *uniq_moved.first() != 3) {
        return false;
    }
    uniq = std::move(uniq_moved);
    if (!uniq.first() || *uniq.first() != 3 || uniq_moved.first()) {
        return false;
    }

    int value = 5;
    compressed_pair<int*, int> ptr{ &value, 6 };
    compressed_pair<int*, int> ptr_copy{ ptr };
    if (ptr_copy.first() != &value || ptr.first() != &value) {
        return false;
    }

    counted::reset();
    {
        compressed_pair<counted, counted> cnt{ 1, 2 };
        compressed_pair<counted, counted> cnt_copy{ cnt };
        compressed_pair<counted, counted> cnt_move{ std::move(cnt) };
        cnt_copy = cnt_move;
        cnt      = std::move(cnt_copy);
        if (cnt.first().value != 1 || cnt.second().value != 2 ||
            counted::live != 6) {
            return false;
        }
    }
    return counted::live == 0 && counted::destroy == 6;
}

bool test_access() {
    compressed_pair<int, std::string> pair{ 1, "a" };
    pair.first()  = 2;
    pair.second() = "b";
    if (pair.get<0>() != 2 || pair.get<1>() != "b") {
        return false;
    }
    pair.get<0>() = 3;
    if (&pair.get<1>() != &pair.second()) {
        return false;
    }

    auto& [first, second] = pair;
    first                 = 4;
    second += "c";
    if (pair.first() != 4 || pair.second() != "bc") {
        return false;
    }

    const auto& cref              = pair;
    const auto& [cfirst, csecond] = cref;
    static_assert(std::is_same_v<decltype(cfirst), const int>);
    if (&csecond != &pair.second()) {
        return false;
    }

    auto [vfirst, vsecond] = pair;
    vsecond                = "other";
    if (vfirst != 4 || pair.second() != "bc") {
        return false;
    }

    std::string taken = std::move(pair).second();
    return taken == "bc";
}

bool test_to_std_pair() {
    const compressed_pair<int, std::string> pair{ 1, "abc" };
    std::pair<int, std::string> copied = pair;
    std::pair<int, std::string> moved =
        compressed_pair<int, std::string>{ 2, "def" };
    return copied.first == 1 && copied.second == "abc" &&
           pair.second() == "abc" && moved.first == 2 && moved.second == "def";
}

bool test_compare() {
    compressed_pair<int, std::string> lhs{ 1, "a" };
    compressed_pair<int, std::string> rhs{ 1, "a" };
    compressed_pair<int, std::string> other{ 1, "b" };
    std::pair<int, std::string> spair{ 1, "a" };
    std::pair<int, std::string> sother{ 2, "a" };
    return lhs == rhs && !(lhs != rhs) && lhs != other && !(lhs == other) &&
           lhs == spair && !(lhs != spair) && lhs != sother && !(lhs == sother);
}

} // namespace

int main() {
    if (!test_default_construct()) {
        return 1;
    }
    if (!test_value_construct()) {
        return 2;
    }
    if (!test_piecewise()) {
        return 3;
    }
    if (!test_from_std_pair()) {
        return 4;
    }
    if (!test_from_pair_like()) {
        return 5;
    }
    if (!test_copy_move()) {
        return 6;
    }
    if (!test_access()) {
        return 7;
    }
    if (!test_to_std_pair()) {
        return 8;
    }
    if (!test_compare()) {
        return 9;
    }
    return 0;
}
