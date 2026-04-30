#pragma once
#include <cstddef>
#include <tuple>
#include <utility>
#include "neutron/detail/ecs/metainfo.hpp"
#include "neutron/detail/macros.hpp"
#include "neutron/execution.hpp"

namespace neutron {

template <auto... Worlds>
consteval auto split() noexcept {
    return std::tuple<>{};
}

template <execution::scheduler Sch, typename Worlds>
class runtime {
public:
    runtime(const Sch& sch) : sch_(sch) {}

    runtime(Sch&& sch, Worlds& worlds) : sch_(std::move(sch)) {}

    int run() {
        using namespace execution;
        using enum forward_progress_guarantee;

        const auto guarantee = get_forward_progress_guarantee(sch_);
        if (guarantee == forward_progress_guarantee::weakly_parallel) {
            //
        } else {
            _run();
        }

        return 0;
    }

private:
    void _run() {
        auto alloc = get_allocator(sch_);
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            //
        }(std::make_index_sequence<std::tuple_size_v<Worlds>>());
    }

    Sch sch_;
    Worlds worlds_;
};

template <auto... Worlds, execution::scheduler Sch>
ATOM_NODISCARD auto make_runtime(Sch&& sch) {
    using worlds_t = std::remove_pointer_t<decltype(split<Worlds...>())>;
    return runtime<std::decay_t<Sch>, worlds_t>(std::forward<Sch>(sch));
}

} // namespace neutron
