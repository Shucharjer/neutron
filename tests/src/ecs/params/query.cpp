#include <concepts>
#include <neutron/ecs.hpp>
#include <neutron/metafn.hpp>

using namespace neutron;
using enum stage;

template <>
constexpr bool ::neutron::as_component<int> = true;

void qfn(query<with<int>> qry) {}

int main() {
    {
        //
    }

    {
        using query_with_int_t = query<with<int>>;
        using spec             = param_spec<query_with_int_t>;
        using with_comps       = spec::with_comps;
        using withany_comps    = spec::withany_comps;

        using requested_with_comps = spec::accessibilities;
        using accessibility_of_int =
            type_list_element_t<0, requested_with_comps>;
        static_assert(
            std::same_as<accessibility_of_int, request_accessibility<int>>);
    }

    {
        using query_with_int_t = query<with<int&>>;
        using spec             = param_spec<query_with_int_t>;
        using with_comps       = spec::with_comps;
        using withany_comps    = spec::withany_comps;

        using requested_with_comps = spec::accessibilities;
        using accessibility_of_int =
            type_list_element_t<0, requested_with_comps>;
        static_assert(
            std::same_as<accessibility_of_int, request_accessibility<int&>>);
    }

    {
        using query_with_int_t = query<with<const int&>>;
        using spec             = param_spec<query_with_int_t>;
        using with_comps       = spec::with_comps;
        using withany_comps    = spec::withany_comps;

        using requested_with_comps = spec::accessibilities;
        using accessibility_of_int =
            type_list_element_t<0, requested_with_comps>;
        static_assert(std::same_as<
                      accessibility_of_int, request_accessibility<const int&>>);
    }

    return 0;
}
