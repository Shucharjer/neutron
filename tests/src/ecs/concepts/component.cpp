#ifdef ATOM_USES_MODULES
import neutron;
#else
    #include <neutron/ecs.hpp>
#endif

using namespace neutron;


struct not_comp {};
static_assert(!component<not_comp>);
static_assert(!gpu_component<not_comp>);

struct tag_comp {
    using component_concept = component_tag;
};
static_assert(component<tag_comp>);
static_assert(!gpu_component<tag_comp>);

struct as_comp {};
template <>
constexpr bool neutron::as_component<as_comp> = true;
static_assert(component<as_comp>);
static_assert(!gpu_component<as_comp>);

struct tag_gpu_comp {
    using component_concept = gpu_component_tag;
};
static_assert(component<tag_gpu_comp>);
static_assert(gpu_component<tag_gpu_comp>);

struct as_gpu_comp {};
template <>
constexpr bool neutron::as_component<as_gpu_comp> = true;
template <>
constexpr bool neutron::as_gpu_component<as_gpu_comp> = true;
static_assert(component<as_gpu_comp>);
static_assert(gpu_component<as_gpu_comp>);

int main() { return 0; }
