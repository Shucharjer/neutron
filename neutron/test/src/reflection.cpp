#include <cstdlib>
#include <neutron/reflection.hpp>
#include <neutron/utility.hpp>
#include "require.hpp"

void test_member_count_of();
void test_get();
void test_member_names_of();
void test_description_of();
void test_reflected();
void test_offsets();
void test_offset_value();
void test_member_type_of();

int main() {
    test_member_count_of();
    test_get();
    test_member_names_of();
    test_description_of();
    test_reflected();
    test_offsets();
    test_offset_value();
    test_member_type_of();
}

using namespace neutron;

struct aggregate {
    int member1;
    char member2;
};

class not_aggregate {
public:
    constexpr not_aggregate(int var1, char var2) noexcept
        : another_member1_(var1), another_member2_(var2) {}
    constexpr not_aggregate(const not_aggregate&)            = default;
    constexpr not_aggregate(not_aggregate&&)                 = default;
    constexpr not_aggregate& operator=(const not_aggregate&) = default;
    constexpr not_aggregate& operator=(not_aggregate&&)      = default;
    constexpr ~not_aggregate()                               = default;

    REFL_MEMBERS(not_aggregate, another_member1_, another_member2_)
private:
    int another_member1_;
    char another_member2_;
};

void test_member_count_of() {
    require(member_count_of<aggregate>() == 2);
    require(member_count_of<not_aggregate>() == 2);
}
void test_get() {
    const auto num = 114514;
    const char cha = '!';
    auto agg       = aggregate{ .member1 = num, .member2 = cha };
    auto nagg      = not_aggregate{ num, cha };

    require(get<0>(agg) == num);
    require(get<1>(agg) == cha);

    require(get<0>(nagg) == num);
    require(get<1>(nagg) == cha);

    require(get_by_name<"member1">(agg) == num);
    require(get_by_name<"member2">(agg) == cha);

    require(get_by_name<"another_member1_">(nagg) == num);
    require(get_by_name<"another_member2_">(nagg) == cha);
}

void test_member_names_of() {
    auto names_a  = member_names_of<aggregate>();
    auto names_na = member_names_of<not_aggregate>();

    require(names_a[0] == "member1");
    require(names_a[1] == "member2");

    require(names_na[0] == "another_member1_");
    require(names_na[1] == "another_member2_");
}

void test_description_of() {
    auto description_a  = traits_of<aggregate>();
    auto description_na = traits_of<not_aggregate>();

    using enum traits_bits;

    require(authenticity_of({ .desc = description_a, .bits = is_aggregate }));
    require(!authenticity_of({ .desc = description_na, .bits = is_aggregate }));
}
void test_reflected() {
    auto reflected_a  = reflected<aggregate>();
    auto reflected_na = reflected<not_aggregate>();

    using enum traits_bits;

    auto description_a  = reflected_a.traits();
    auto description_na = reflected_na.traits();

    require(authenticity_of({ .desc = description_a, .bits = is_aggregate }));
    require_false(
        authenticity_of({ .desc = description_na, .bits = is_aggregate }));
}
void test_offsets() {
    auto offsets = offsets_of<aggregate>();
    auto ptr     = std::get<1>(offsets);
    auto agg     = aggregate{};
    agg.member2  = 'c';
    require(agg.*ptr == 'c');
    agg.*ptr = 'b';
    require(agg.member2 == 'b');
}
void test_offset_value() {
    require((offset_value_of<0, aggregate>() == 0));
    require((offset_value_of<1, aggregate>() == 4));

    // TODO:
    // require((offset_value_of<0, not_aggregate>() == 0))
    // require((offset_value_of<1, not_aggregate>() == 4))
}

void test_member_type_of() {
    require((std::same_as<int, member_type_of_t<0, aggregate>>));
    require((std::same_as<char, member_type_of_t<1, aggregate>>));

    require((std::same_as<int, member_type_of_t<0, not_aggregate>>));
    require((std::same_as<char, member_type_of_t<1, not_aggregate>>));
}
