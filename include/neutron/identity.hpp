#pragma once

#if defined(ATOM_USES_MODULES) && !defined(ATOM_BUILDING_MODULE)
    #include "neutron/macros.hpp"
#else

    #ifndef ATOM_BUILDING_MODULE
        #include <atomic>
    #endif
    #include "neutron/detail/utility/id_t.hpp"
    #include "neutron/detail/utility/macros.hpp"

namespace neutron {

ATOM_EXPORT class type_identity {
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

ATOM_EXPORT class non_type_identity {
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

#endif
