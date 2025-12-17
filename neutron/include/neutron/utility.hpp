#pragma once
#include "../src/neutron/internal/utility/get.hpp"  // IWYU pragma: export
#include "../src/neutron/internal/utility/id_t.hpp" // IWYU pragma: export
#include "../src/neutron/internal/utility/immediately.hpp" // IWYU pragma: export
#include "../src/neutron/internal/utility/spreader.hpp" // IWYU pragma: export

namespace neutron {

template <typename Derived>
class singleton {
public:
    using value_type                       = Derived;
    using self_type                        = singleton;
    singleton(const singleton&)            = delete;
    singleton(singleton&&)                 = delete;
    singleton& operator=(const singleton&) = delete;
    singleton& operator=(singleton&&)      = delete;

    [[nodiscard]] static auto& instance() {
        static Derived inst;
        return inst;
    }

protected:
    singleton() noexcept  = default;
    ~singleton() noexcept = default;
};

} // namespace neutron
