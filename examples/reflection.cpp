// Inspect the members of a plain aggregate without annotating it.
#include <array>
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
    // bitstream is a cursor over caller-provided storage.
    alignas(std::max_align_t) std::array<std::byte, 64> buffer{};
    neutron::bitstream out{ buffer.data(), buffer.size() };
    neutron::serialize(out, p);

    player another{};
    neutron::bitstream in{ buffer.data(), buffer.size() };
    neutron::deserialize(in, another);
    neutron::println(
        "{}, {}, {}", another.id == p.id, another.health == p.health,
        another.name == p.name);
}

int main() {
    player p{ .id = 1, .health = 87.5F, .name = "neutron" };
    dump(p);
    serialize_and_deserialize(p);
}
