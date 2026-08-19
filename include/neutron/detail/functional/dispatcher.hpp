// IWYU pragma: private, include <neutron/functional.hpp>
#pragma once
#include <memory>
#include "neutron/detail/functional/sink.hpp"
#include "neutron/detail/memory/rebind_alloc.hpp"

namespace neutron {

template <typename Alloc = std::allocator<void>>
class dispatcher {
    template <typename T>
    using _allocator_t = rebind_alloc_t<Alloc, T>;

public:
    using _hash_t   = uint32_t;
    using _hash_map = flat_hash_map<
        _hash_t, _sink_base, std::hash<_hash_t>, std::equal_to<_hash_t>,
        _allocator_t<std::pair<_hash_t, _sink_base*>>>;

    dispatcher() = default;

    template <typename Al = Alloc>
    requires std::constructible_from<_hash_map, Alloc>
    dispatcher(const Al& alloc) : sinks_(alloc) {}

    dispatcher(const dispatcher&)            = delete;
    dispatcher& operator=(const dispatcher&) = delete;
    dispatcher(dispatcher&&)                 = delete;
    dispatcher& operator=(dispatcher&&)      = delete;

    ~dispatcher() {
        for (auto [hash, ptr] : sinks_) {
            ptr->destroy(ptr);
            ::delete ptr;
        }
    }

    template <typename T>
    sink<T>& sink() {
        using sink_t        = ::neutron::sink<T>;
        constexpr auto hash = hash_of<T>();
        auto it             = sinks_.find(hash);
        if (it == sinks_.end()) [[unlikely]] {
            auto [iter, succ] = sinks_.emplace(hash, ::new sink_t);
            it                = iter;
        }

        return static_cast<sink_t*>(it->second);
    }

private:
    _hash_map sinks_;
};

} // namespace neutron
