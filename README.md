# Neutron

Language: English | [简体中文](docs/zh/readme.md)

Neutron is a header-only C++20 utility library for data-oriented and engine-style
code. The project currently centers on an archetype-based ECS and also provides
reflection helpers, sender/receiver-style execution utilities, compact
containers, and memory-oriented building blocks that are useful in low-level
runtime code.

## Highlights

- Header-only and easy to embed in an existing CMake project.
- Archetype-based ECS with entity recycling, batch spawn/kill, and deferred
  structural edits.
- Declarative world descriptors with staged systems and compile-time metadata.
- Compile-time validation for ordering constraints and a range of ECS access
  conflicts.
- Execution utilities that can forward to standard facilities or `stdexec`
  when available, while keeping an in-tree fallback.
- Reflection helpers for aggregates and opt-in support for non-aggregate
  types, with a more advanced direction planned for C++26-era reflection.
- Utility containers such as `smvec`, `shift_map`, `sparse_map`,
  `flat_hash_map`, and `object_pool`.

## Requirements

- A C++20 compiler
- CMake 3.21 or newer
- If you build with Clang through the provided CMake target, Neutron enables
  `-fexperimental-library` automatically so `std::execution::par` and newer
  execution-related facilities remain usable, especially on libc++ setups
  where some versions still require that flag.

## Integrating Neutron

The simplest integration path is to add the directory and link the interface
target:

```cmake
add_subdirectory(${NEUTRON_ROOT_DIR})

# target_link_libraries(my_target PRIVATE atom::neutron)
target_link_libraries(my_target PRIVATE neutron)
```

Neutron exposes the `neutron` interface target and the `atom::neutron` alias.
Because the library is header-only, linking mainly propagates include
directories and required compile options.

## ECS Quick Start

The ECS API lives in [`<neutron/ecs.hpp>`](include/neutron/ecs.hpp). Components
are plain C++ types tagged with `component_concept`, systems are regular
functions, and world descriptors define how those systems are scheduled.

```cpp
#include <cstddef>
#include <memory>
#include <neutron/ecs.hpp>

struct Position {
    using component_concept = neutron::component_t;
    float x{};
    float y{};
};

struct Velocity {
    using component_concept = neutron::component_t;
    float x{};
    float y{};
};

template <typename... Filters>
using query = neutron::basic_querior<std::allocator<std::byte>, 8, Filters...>;

void integrate(query<neutron::with<Position&, Velocity&>> movers) {
    for (auto&& [position, velocity] : movers.get()) {
        position.x += velocity.x;
        position.y += velocity.y;
    }
}

constexpr auto game_world =
    neutron::world_desc |
    neutron::add_systems<neutron::stage::update, &integrate>;

int main() {
    auto world = neutron::make_world<game_world>();

    world.spawn(Position{0.0f, 0.0f}, Velocity{1.0f, 0.5f});
    world.spawn(Position{10.0f, -3.0f});

    neutron::call_update(world);
}
```

What this example demonstrates:

- `component_concept` marks a type as an ECS component.
- `basic_querior` can be constructed from a world and filtered with
  `with`, `without`, and `withany`.
- `call_update(world)` drives the `pre_update`, `update`, and `post_update`
  stages for a world.

## Direct World Operations

In most cases this is not the recommended layer unless you know exactly why you
want direct storage access.

If you only want the storage layer, `world_base` and
`basic_world<world_descriptor_t<>>` also support direct structural editing:

```cpp
neutron::world_base<> world;

auto entity = world.spawn(Position{1.0f, 2.0f});
world.add_components(entity, Velocity{3.0f, 4.0f});
world.remove_components<Velocity>(entity);

auto batch = world.spawn_n(1024, Position{}, Velocity{});
world.kill(batch);
```

For deferred structural edits, use `neutron::command_buffer<>`:

```cpp
neutron::world_base<> world;
neutron::command_buffer<> commands;

auto future = commands.spawn(Position{1.0f, 2.0f});
commands.add_components(future, Velocity{3.0f, 4.0f});
commands.apply(world);
```

If you do want a scheduled world, prefer `make_world<descriptor>()` over
spelling `basic_world<decltype(descriptor)>` directly unless you specifically
need the lower-level type.

The direct API also includes `reserve`, `spawn_n`, `is_alive`, `kill`, and
component add/remove operations for batch-oriented workflows.

## Descriptors, Scheduling, and System Inputs

World descriptors are built by composing small adapters:

- `add_systems<stage, ...>` registers systems for a stage.
- `execute<group<N>>` assigns a grouped execution order.
- `execute<individual>` lets a world run on its own execution path.
- `execute<frequency<...>>` and `execute<dynamic_frequency<>>` control update
  cadence.
- `enable_render` and `enable_events` attach render and event metadata.

Systems can request more than just component queries. Neutron also supports:

- `basic_commands<>` for structural edits through command buffers
- `res<T...>` for resources owned by the world
- `global<T...>` for externally bound shared state
- `local<T...>` for per-system local storage

For multi-world execution, the runtime helpers in
[`<neutron/ecs.hpp>`](include/neutron/ecs.hpp) and
[`<neutron/execution_resources.hpp>`](include/neutron/execution_resources.hpp)
cover patterns such as `make_worlds`, `call_update(scheduler, command_buffers,
worlds)`, and `make_world_schedule(...)`.

The descriptor system derives metadata at compile time, which helps reduce
runtime overhead.

For a more complete end-to-end example, take a look at
[`app.hpp`](test/include/app.hpp) and the repository's
[`example/src/main.cpp`](../example/src/main.cpp). The example wires Neutron
into a real application flow with rendering and an ImGui-backed "hello world"
window.

## Other Public Modules

### Execution

[`<neutron/execution.hpp>`](include/neutron/execution.hpp) exposes a
sender/receiver-style execution layer. If a standard implementation or
`stdexec` is available, Neutron forwards to it; otherwise the in-tree fallback
is used.

```cpp
#include <neutron/execution.hpp>

auto sndr = neutron::execution::just(40)
          | neutron::execution::then([](int value) { return value + 2; });

auto [answer] = neutron::this_thread::sync_wait(std::move(sndr)).value();
```

### Reflection

[`<neutron/reflection.hpp>`](include/neutron/reflection.hpp) provides
compile-time member inspection for aggregates:

```cpp
#include <neutron/reflection.hpp>

struct Vec2 {
    float x;
    float y;
};

static_assert(neutron::member_count_of<Vec2>() == 2);
constexpr auto names = neutron::member_names_of<Vec2>();
```

For non-aggregate types, opt in with `REFL_MEMBERS(Type, ...)`.

### Containers and Utilities

Useful entry points include:

- [`<neutron/smvec.hpp>`](include/neutron/smvec.hpp): a vector-like container
  with inline storage.
- [`<neutron/sparse_map.hpp>`](include/neutron/sparse_map.hpp): a very fast
  associative container that trades memory for speed.
- [`<neutron/shift_map.hpp>`](include/neutron/shift_map.hpp): a `sparse_map`
  variant optimized for index-oriented storage and generation-aware
  identifiers.
- [`<neutron/flat_hash_map.hpp>`](include/neutron/flat_hash_map.hpp): a
  general-purpose high-performance associative container.
- [`<neutron/object_pool.hpp>`](include/neutron/object_pool.hpp): pooled
  object storage utilities.
- [`<neutron/polymorphic.hpp>`](include/neutron/polymorphic.hpp) for
  lightweight type-erased polymorphism.
- [`<neutron/ranges.hpp>`](include/neutron/ranges.hpp): range helpers such as
  `ranges::to`, adaptor-closure support, and related concepts.
- [`<neutron/memory.hpp>`](include/neutron/memory.hpp): storage-aware memory
  helpers including `uninitialized_*` building blocks.
- [`<neutron/functional.hpp>`](include/neutron/functional.hpp): low-overhead
  callable and dispatch utilities.

## Building Tests and Benchmarks

Neutron can be configured as a standalone CMake project:

```bash
cmake -S third_party/neutron -B build/neutron -DBUILD_NEUTRON_TESTING=ON
cmake --build build/neutron
ctest --test-dir build/neutron
```

To enable benchmarks, add `-DBUILD_NEUTRON_BENCHMARK=ON`. The benchmark build
uses Google Benchmark through CMake `FetchContent`.

## License

Neutron is released under the MIT License. See [LICENSE](LICENSE) for details.
