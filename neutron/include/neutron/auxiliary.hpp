#pragma once

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
