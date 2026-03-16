// IWYU pragma: private, include <neutron/ecs.hpp>
#pragma once
#include "neutron/detail/ecs/fwd.hpp"

#include <concepts>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>
#include "neutron/detail/ecs/world_base.hpp"

#ifndef neutron_STD_FUNCTION_CMDBUF
    #include <bit>
#else
    #include <functional>
#endif

namespace neutron {

#ifndef neutron_STD_FUNCTION_CMDBUF

// The code is not fully tested yet, it's useful currently.
// This version may be a little faster than std::function version.
// From one hand, we hold the pointers of memory blocks, which means it will
// spend less when vector relocating its elements.
// From another hand, we do not use virtual function.

namespace _command {

template <typename Alloc>
class _command_base {
    template <typename Ty>
    using _vector_t = std::vector<Ty, rebind_alloc_t<Alloc, Ty>>;

public:
    using future_map_t = _vector_t<entity_t>;

    constexpr _command_base(
        void (*cmd)(void* payload, world_base<Alloc>&, future_map_t&),
        void (*destroy)(void*)) noexcept
        : command_(cmd), destroy_(destroy) {}

    _command_base(const _command_base&)            = delete;
    _command_base& operator=(const _command_base&) = delete;
    _command_base(_command_base&&)                 = delete;
    _command_base& operator=(_command_base&&)      = delete;

    ~_command_base() noexcept = default;

    void operator()(
        world_base<Alloc>& world,
        [[maybe_unused]] _vector_t<entity_t>& future_map) {
        command_(this, world, future_map);
    }

    static void destroy(_command_base* self) { self->destroy_(self); }

private:
    void (*command_)(
        void* payload, world_base<Alloc>& world, future_map_t& future_map);
    void (*destroy_)(void* ptr);
};

template <typename Derived, typename Alloc>
class _command_impl_base : _command_base<Alloc> {
public:
    using future_map_t = typename _command_base<Alloc>::future_map_t;
    _command_impl_base() noexcept : _command_base<Alloc>(&_invoke, &_destroy) {}

private:
    static void _invoke(
        void* payload, world_base<Alloc>& world, future_map_t& future_map) {
        static_cast<Derived*>(payload)->invoke(world, future_map);
    }

    static void _destroy(void* payload) noexcept(
        std::is_nothrow_destructible_v<Derived>) {
        static_cast<Derived*>(payload)->~Derived();
    }
};

template <typename Alloc>
class _spawn : _command_impl_base<_spawn<Alloc>, Alloc> {
public:
    using future_map_t = typename _command_base<Alloc>::future_map_t;

    constexpr _spawn(future_entity_t fut) noexcept : fut_(fut) {}

    void invoke(
        world_base<Alloc>& world, [[maybe_unused]] future_map_t& future_map) {
        future_map[fut_.get()] = world.spawn();
    }

private:
    future_entity_t fut_;
};

template <typename Alloc, component... Components>
class _spawn_with_comps :
    _command_impl_base<_spawn_with_comps<Alloc, Components...>, Alloc> {
public:
    using future_map_t = typename _command_base<Alloc>::future_map_t;

    constexpr _spawn_with_comps(future_entity_t fut) noexcept : fut_(fut) {}

    void invoke(world_base<Alloc>& world, future_map_t& future_map) {
        future_map[fut_.get()] = world.template spawn<Components...>();
    }

private:
    future_entity_t fut_;
};

template <typename Alloc, component... Components>
class _spawn_with_value :
    _command_impl_base<_spawn_with_value<Alloc, Components...>, Alloc> {
public:
    using future_map_t = typename _command_base<Alloc>::future_map_t;

    template <typename... Comps>
    _spawn_with_value(future_entity_t fut, Comps&&... comps) noexcept
        : fut_(fut), comps_(std::forward<Comps>(comps)...) {}

    void invoke(
        world_base<Alloc>& world, [[maybe_unused]] future_map_t& future_map) {
        future_map[fut_.get()] = std::apply(
            [&world](auto&&... comps) {
                return world.spawn(std::forward<decltype(comps)>(comps)...);
            },
            std::move(comps_));
    }

private:
    future_entity_t fut_;
    std::tuple<Components...> comps_;
};

template <typename Alloc, component... Components>
class _add_comps_for_fut :
    _command_impl_base<_add_comps_for_fut<Alloc, Components...>, Alloc> {
public:
    using future_map_t = typename _command_base<Alloc>::future_map_t;

    constexpr _add_comps_for_fut(future_entity_t fut) : fut_(fut) {}

    void invoke(world_base<Alloc>& world, future_map_t& future_map) {
        const auto entity = fut_.get();
        world.template add_components<Components...>(entity);
    }

private:
    future_entity_t fut_;
};

template <typename Alloc, component... Components>
class _add_comps : _command_impl_base<_add_comps<Alloc, Components...>, Alloc> {
public:
    using future_map_t = typename _command_base<Alloc>::future_map_t;

    constexpr _add_comps(entity_t entity) : entity_(entity) {}

    void invoke(world_base<Alloc>& world, future_map_t& future_map) {
        world.template add_components<Components...>(entity_);
    }

private:
    entity_t entity_;
};

template <typename Alloc, component... Components>
class _add_comps_for_fut_with_value :
    _command_impl_base<
        _add_comps_for_fut_with_value<Alloc, Components...>, Alloc> {
public:
    using future_map_t = typename _command_base<Alloc>::future_map_t;

    template <component... Comps>
    constexpr _add_comps_for_fut_with_value(
        future_entity_t fut, Comps&&... components)
        : fut_(fut), comps_(std::forward<Comps>(components)...) {}

    void invoke(world_base<Alloc>& world, future_map_t& future_map) {
        const auto entity = fut_.get();
        std::apply(
            [entity, &world](auto&&... comps) {
                world.add_components(
                    entity, std::forward<decltype(comps)>(comps)...);
            },
            std::move(comps_));
    }

private:
    future_entity_t fut_;
    std::tuple<Components...> comps_;
};

template <typename Alloc, component... Components>
class _add_comps_with_value :
    _command_impl_base<_add_comps_with_value<Alloc, Components...>, Alloc> {
public:
    using future_map_t = typename _command_base<Alloc>::future_map_t;

    template <component... Comps>
    constexpr _add_comps_with_value(entity_t entity, Comps&&... components)
        : entity_(entity), comps_(std::forward<Comps>(components)...) {}

    void invoke(world_base<Alloc>& world, future_map_t& future_map) {
        const auto entity = entity_;
        std::apply(
            [entity, &world](auto&&... comps) {
                world.add_components(
                    entity, std::forward<decltype(comps)>(comps)...);
            },
            std::move(comps_));
    }

private:
    entity_t entity_;
    std::tuple<Components...> comps_;
};

template <typename Alloc, component... Components>
class _remove_comps_for_fut :
    _command_impl_base<_remove_comps_for_fut<Alloc, Components...>, Alloc> {
public:
    using future_map_t = typename _command_base<Alloc>::future_map_t;

    _remove_comps_for_fut(future_entity_t fut) noexcept : fut_(fut) {}

    void invoke(world_base<Alloc>& world, future_map_t& future_map) {
        const auto entity = future_map[fut_.get()];
        world.template remove_components<Components...>(entity);
    }

private:
    future_entity_t fut_;
};

template <typename Alloc, component... Components>
class _remove_comps :
    _command_impl_base<_remove_comps<Alloc, Components...>, Alloc> {
public:
    using future_map_t = typename _command_base<Alloc>::future_map_t;

    _remove_comps(entity_t entity) noexcept : entity_(entity) {}

    void invoke(world_base<Alloc>& world, future_map_t& future_map) {
        world.template remove_components<Components...>(entity_);
    }

private:
    entity_t entity_;
};

template <typename Alloc>
class _kill_fut : _command_impl_base<_kill_fut<Alloc>, Alloc> {
public:
    using future_map_t = typename _command_base<Alloc>::future_map_t;

    _kill_fut(future_entity_t fut) noexcept : fut_(fut) {}

    void invoke(world_base<Alloc>& world, future_map_t& future_map) noexcept {
        const auto eneity = future_map[fut_.get()];
        world.kill(eneity);
    }

private:
    future_entity_t fut_;
};

template <typename Alloc>
class _kill : _command_impl_base<_kill<Alloc>, Alloc> {
public:
    using future_map_t = typename _command_base<Alloc>::future_map_t;

    _kill(entity_t entity) noexcept : entity_(entity) {}

    void invoke(world_base<Alloc>& world, future_map_t& future_map) {
        world.kill(entity_);
    }

private:
    entity_t entity_;
};

} // namespace _command

template <std_simple_allocator Alloc>
class alignas(std::hardware_destructive_interference_size) command_buffer {
    template <typename Ty>
    using _allocator_t = neutron::rebind_alloc_t<Alloc, Ty>;

    template <typename Ty>
    using _vector_t = std::vector<Ty, _allocator_t<Ty>>;

    using _world_base = world_base<Alloc>;

    using _command_base = _command::_command_base<Alloc>;

    static constexpr size_t block_size  = 1024U << 4UL; // 16kb
    static constexpr auto default_align = static_cast<std::align_val_t>(16);

    struct _ptr_deletor {
        constexpr void operator()(std::byte* ptr) noexcept {
            ::operator delete(ptr, default_align);
        }
    };

    using _unique_ptr = std::unique_ptr<std::byte[], _ptr_deletor>; // NOLINT

public:
    using allocator_type = Alloc;

    template <typename Al = Alloc>
    ATOM_CONSTEXPR_SINCE_CXX23 command_buffer(const Al& alloc = {})
        : commands_(_allocator_t<_command_base*>{ alloc }),
          buffers_(_allocator_t<_unique_ptr>{ alloc }) {
        std::byte* ptr = nullptr;
        try {
            ptr = static_cast<std::byte*>(
                ::operator new(block_size, default_align));
            buffers_.emplace_back(ptr, _ptr_deletor{});
        } catch (...) {
            ::operator delete(ptr, default_align);
            throw;
        }
    }

    constexpr Alloc get_allocator() noexcept {
        return commands_.get_allocator();
    }

    constexpr void reset() noexcept {
        inframe_index_ = 0;
        commands_.clear();
        current_ = 0;
        offset_  = 0;
    }

    future_entity_t spawn() {
        using command = _command::_spawn<Alloc>;

        auto* const ptr = _assure<command>();
        const auto fut  = future_entity_t{ inframe_index_++ };
        ::new (ptr) command{ fut };
        commands_.emplace_back(ptr);
        return fut;
    }

    template <component... Components>
    requires(std::same_as<Components, std::remove_cvref_t<Components>> && ...)
    future_entity_t spawn() {
        using command = _command::_spawn_with_comps<Alloc, Components...>;

        auto* const ptr = _assure<command>();
        const auto fut  = future_entity_t{ inframe_index_++ };
        ::new (ptr) command{ fut };
        commands_.emplace_back(ptr);
        return fut;
    }

    template <component... Components>
    future_entity_t spawn(Components&&... components) {
        using command = _command::_spawn_with_value<
            Alloc, std::remove_cvref_t<Components>...>;

        auto* const ptr = _assure<command>();
        const auto fut  = future_entity_t{ inframe_index_++ };
        ::new (ptr) command{ fut, std::forward<Components>(components)... };
        commands_.emplace_back(ptr);
        return fut;
    }

    template <component... Components>
    requires(std::same_as<Components, std::remove_cvref_t<Components>> && ...)
    void add_components(future_entity_t entity) {
        using command = _command::_add_comps_for_fut<Alloc, Components...>;

        auto* const ptr = _assure<command>();
        ::new (ptr) command{ entity };
        commands_.emplace_back(ptr);
    }

    template <component... Components>
    void add_components(future_entity_t entity, Components&&... components) {
        using command = _command::_add_comps_for_fut_with_value<
            Alloc, std::remove_cvref_t<Components>...>;

        auto* const ptr = _assure<command>();
        ::new (ptr) command{ entity, std::forward<Components>(components)... };
        commands_.emplace_back(ptr);
    }

    template <component... Components>
    requires(std::same_as<Components, std::remove_cvref_t<Components>> && ...)
    void add_components(entity_t entity) {
        using command = _command::_add_comps<Alloc, Components...>;

        auto* const ptr = _assure<command>();
        ::new (ptr) command{ entity };
        commands_.emplace_back(ptr);
    }

    template <component... Components>
    void add_components(entity_t entity, Components&&... components) {
        using command = _command::_add_comps_with_value<
            Alloc, std::remove_cvref_t<Components>...>;

        auto* const ptr = _assure<command>();
        ::new (ptr) command{ entity, std::forward<Components>(components)... };
        commands_.emplace_back(ptr);
    }

    template <component... Components>
    requires(std::same_as<Components, std::remove_cvref_t<Components>> && ...)
    void remove_components(future_entity_t entity) {
        using command = _command::_remove_comps_for_fut<Alloc, Components...>;

        auto* const ptr = _assure<command>();
        ::new (ptr) command{ entity };
        commands_.emplace_back(ptr);
    }

    template <component... Components>
    requires(std::same_as<Components, std::remove_cvref_t<Components>> && ...)
    void remove_components(entity_t entity) {
        using command = _command::_remove_comps<Alloc, Components...>;

        auto* const ptr = _assure<command>();
        ::new (ptr) command{ entity };
        commands_.emplace_back(ptr);
    }

    void kill(future_entity_t entity) {
        using command = _command::_kill_fut<Alloc>;

        auto* const ptr = _assure<command>();
        ::new (ptr) command{ entity };
        commands_.emplace_back(ptr);
    }

    void kill(entity_t entity) {
        using command = _command::_kill<Alloc>;

        auto* const ptr = _assure<command>();
        ::new (ptr) command{ entity };
        commands_.emplace_back(ptr);
    }

    void apply(world_base<Alloc>& world) {
        _vector_t<entity_t> future_map(
            inframe_index_, commands_.get_allocator());

        for (_command_base* cmd : commands_) {
            (*cmd)(world, future_map);
            _command_base::destroy(cmd);
        }
    }

private:
    template <size_t Align>
    std::byte* _next_aligned(std::byte* ptr) noexcept {
        constexpr auto magic = Align - 1;
        return std::bit_cast<std::byte*>(
            (std::bit_cast<uintptr_t>(ptr) + magic) & ~magic);
    }

    /**
     * @return std::byte* A pointer refers to a constrcutible addr in buffer for
     * `Command`.
     */
    template <typename Command>
    std::byte* _assure_impl() {
        constexpr auto size  = sizeof(Command);
        constexpr auto align = alignof(Command);

        static_assert(
            size <= (block_size >> 1),
            "too many components or some components have huge inner storage");

        auto& current = buffers_[current_];
        auto* ptr     = current.get();

        auto* aligned = _next_aligned<align>(ptr + offset_);

        if (aligned + size - ptr <= block_size) {
            offset_ = aligned + size - ptr;
            return aligned;
        }

        ++current_;
        if (buffers_.size() == current_) [[unlikely]] {
            ptr = static_cast<std::byte*>(
                ::operator new(block_size, default_align));
            buffers_.emplace_back(ptr, _ptr_deletor{});
            aligned = _next_aligned<align>(ptr);
            offset_ = aligned + size - ptr;
            return aligned;
        } else [[likely]] {
            auto& current = buffers_[current_];
            aligned       = _next_aligned<align>(current.get());
            offset_       = aligned + size - ptr;
            return aligned;
        }
    }

    /**
     * @return std::byte* A pointer refers to a constrcutible addr in buffer for
     * `Command`.
     */
    template <typename Command>
    _command_base* _assure() {
        return reinterpret_cast<_command_base*>(_assure_impl<Command>());
    }

    /// index in one frame.
    index_t inframe_index_{};
    /// current writing command buffer.
    size_t current_{};
    /// offset in current writing command buffer.
    size_t offset_{};
    /// commands pointer
    _vector_t<_command_base*> commands_;
    /// buffers store commands
    _vector_t<_unique_ptr> buffers_;
};

#else

/**
 * @class command_buffer
 * @brief A buffer stores commands in a single thread.
 *
 * @tparam Alloc
 */
template <std_simple_allocator Alloc = std::allocator<std::byte>>
class alignas(std::hardware_destructive_interference_size) command_buffer {

    template <typename Ty>
    using _allocator_t = neutron::rebind_alloc_t<Alloc, Ty>;

    template <typename Ty>
    using _vector_t = std::vector<Ty, _allocator_t<Ty>>;

    using _world_base = world_base<Alloc>;

public:
    template <typename Al = Alloc>
    constexpr command_buffer(const Al& alloc = {}) : commands_(alloc) {}

    constexpr void reset() noexcept {
        inframe_index_ = 0;
        commands_.clear();
    }

    constexpr future_entity_t spawn() noexcept {
        auto fut = future_entity_t{ inframe_index_++ };
        commands_.emplace_back(
            [fut](_world_base& world, _vector_t<entity_t>& future_map) {
                future_map[fut.get()] = world.spawn();
            });
        return fut;
    }

    template <component... Components>
    requires(std::same_as<Components, std::remove_cvref_t<Components>> && ...)
    future_entity_t spawn() {
        const auto fut = future_entity_t{ inframe_index_++ };
        commands_.emplace_back(
            [fut](_world_base& world, _vector_t<entity_t>& future_map) {
                future_map[fut.get()] = world.template spawn<Components...>();
            });
        return fut;
    }

    template <component... Components>
    future_entity_t spawn(Components&&... components) {
        const auto fut = future_entity_t{ inframe_index_++ };
        commands_.emplace_back(
            [fut, ... comps = std::forward<Components>(components)](
                _world_base& world, _vector_t<entity_t>& future_map) {
                future_map[fut.get()] = world.spawn(std::move(comps)...);
            });
        return fut;
    }

    template <component... Components>
    requires(std::same_as<Components, std::remove_cvref_t<Components>> && ...)
    void add_components(future_entity_t entity) {
        commands_.emplace_back(
            [entity](_world_base& world, _vector_t<entity_t>& future_map) {
                const entity_t ntt = future_map[entity.get()];
                world.template add_components<Components...>(ntt);
            });
    }

    template <component... Components>
    requires(std::same_as<Components, std::remove_cvref_t<Components>> && ...)
    void add_components(entity_t entity) {
        commands_.emplace_back(
            [entity](
                _world_base& world, [[maybe_unused]] _vector_t<entity_t>&) {
                world.template add_components<Components...>(entity);
            });
    }

    template <component... Components>
    void add_components(future_entity_t entity, Components&&... comopnents) {
        commands_.emplace_back(
            [entity, ... comps = std::forward<Components>(comopnents)](
                _world_base& world, _vector_t<entity_t>& future_map) {
                const entity_t ntt = future_map[entity.get()];
                world.add_components(ntt, std::move(comps)...);
            });
    }

    template <component... Components>
    void add_components(entity_t entity, Components&&... components) {
        commands_.emplace_back(
            [entity, ... comps = std::forward<Components>(components)](
                _world_base& world, [[maybe_unused]] _vector_t<entity_t>&) {
                world.add_components(entity, std::move(comps)...);
            });
    }

    template <component... Components>
    requires(std::same_as<Components, std::remove_cvref_t<Components>> && ...)
    void remove_components(future_entity_t entity) {
        commands_.emplace_back(
            [entity](_world_base& world, _vector_t<entity_t>& future_map) {
                const entity_t ntt = future_map[entity.get()];
                world.template remove_components<Components...>(ntt);
            });
    }

    template <component... Components>
    requires(std::same_as<Components, std::remove_cvref_t<Components>> && ...)
    void remove_components(entity_t entity) {
        commands_.emplace_back(
            [entity](
                _world_base& world, [[maybe_unused]] _vector_t<entity_t>&) {
                world.template remove_components<Components...>(entity);
            });
    }

    void kill(future_entity_t entity) {
        commands_.emplace_back(
            [entity](
                _world_base& world, _vector_t<entity_t>& future_map) noexcept {
                const entity_t ntt = future_map[entity.get()];
                world.kill(ntt);
            });
    }

    void kill(entity_t entity) {
        commands_.emplace_back(
            [entity](
                _world_base& world,
                [[maybe_unused]] _vector_t<entity_t>&) noexcept {
                world.kill(entity);
            });
    }

    void apply(world_base<Alloc>& world) {
        _vector_t<entity_t> future_map(
            inframe_index_, commands_.get_allocator());

        for (auto& cmd : commands_) {
            cmd(world, future_map);
        }
    }

private:
    index_t inframe_index_{};
    _vector_t<std::function<void(_world_base&, _vector_t<entity_t>&)>>
        commands_;
};

#endif

} // namespace neutron

namespace std {

template <typename Alloc>
struct uses_allocator<neutron::command_buffer<Alloc>, Alloc> :
    std::true_type {};

} // namespace std
