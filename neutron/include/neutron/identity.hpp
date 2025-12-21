#pragma once
#include <atomic>
#include "../src/neutron/internal/utility/id_t.hpp"

namespace neutron {

class type_identity {
    static auto& _atomic() noexcept {
        static std::atomic<id_t> atomic = 0;
        return atomic;
    }

public:
    template <typename Ty, typename Space = void>
    [[nodiscard]] static id_t identity() noexcept {
        static id_t type_id = _atomic().fetch_add(1);
        return type_id;
    }
};

class non_type_identity {
    static auto& _atomic() noexcept {
        static std::atomic<id_t> atomic = 0;
        return atomic;
    }

public:
    template <typename Ty, typename Space = void>
    [[nodiscard]] static id_t identity() noexcept {
        static id_t type_id = _atomic().fetch_add(1);
        return type_id;
    }
};

} // namespace neutron
