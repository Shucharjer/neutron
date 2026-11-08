// Inspect the members of a plain aggregate without annotating it.
#include <cstddef>
#include <string>
#include <tuple>
#include <utility>
#include <neutron/print.hpp>
#include <neutron/reflection.hpp>

struct player {
    int id;
    float health;
    std::string name;
};

static_assert(neutron::member_count_of<player>() == 3);

/// Prints every member as `name = value`.
template <typename Ty>
void dump(const Ty& object) {
    constexpr auto names = neutron::member_names_of<Ty>();
    const auto members   = neutron::offsets_of<Ty>(); // member pointers

    neutron::println("{}:", neutron::name_of<Ty>());
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (neutron::println(
             "  {} = {}", names[Is], object.*std::get<Is>(members)),
         ...);
    }(std::make_index_sequence<names.size()>());
}

void serialize_and_deserialize(const player& p) {
    neutron::bitstream bs;
    neutron::serialize(bs, p);

    player another;
    neutron::deserialize(bs, another);
    neutron::println(
        "{}, {}, {}", another.health == p.health, another.health == p.health,
        another.name == p.name);
}

int main() {
    player p{ .id = 1, .health = 87.5F, .name = "neutron" };
    dump(p);
    serialize_and_deserialize(p);
}
