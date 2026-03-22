# Neutron

语言: [English](../../README.md) | 简体中文

`neutron` 是一个仅包含头文件的 C++20 实用工具库，适用于面向数据和引擎风格的代码。

该项目目前的核心是基于原型的 ECS，同时还提供反射辅助函数、发送/接收式执行工具、紧凑型容器以及适用于底层运行时代码的内存导向构建模块。

## 亮点

- 仅包含头文件，易于嵌入到现有的 CMake 项目中。

- 基于原型的 ECS，支持实体回收、批量生成/销毁以及延迟结构编辑。

- 声明式世界描述符，支持分阶段系统和编译时元数据。

- 编译时验证顺序约束和一系列 ECS 访问冲突。

- 执行工具可以转发到标准工具或 `stdexec`（如果可用），同时保留回退机制。

- 为聚合类型提供反射辅助函数，并可选择支持非聚合类型。我们为C++26提供了更先进的解决方案。

- 实用容器，例如 `smvec`、`shift_map`、`sparse_map`、`flat_hash_map` 和 `object_pool`。

## 要求

- C++20 编译器

- CMake 3.21 或更高版本

- 如果您使用 Clang 通过提供的 CMake 目标进行构建，`neutron` 会自动启用`-fexperimental-library`以获得`std::execution::par`来兼容最新的`execution`特性 (尤其是在使用libcxx的情况下， 部分版本必须使用这一选项来获得相关支持)。

## 集成 Neutron

最简单的集成方法是添加目录并链接接口目标：

```cmake
add_subdirectory(${NEUTRON_ROOT_DIR})

# target_link_libraries(my_target PRIVATE atom::neutron)
target_link_libraries(my_target PRIVATE neutron)
```

neutron 公开了 `neutron` 接口目标和 `atom::neutron` 别名。

由于该库仅包含头文件，因此链接主要传播包含目录和所需的编译选项。

## ECS 快速入门

ECS API 位于 [`<neutron/ecs.hpp>`](../../include/neutron/ecs.hpp)。组件

是带有 `component_concept` 标签的普通 C++ 类型，系统是常规的函数，而世界描述符定义了这些系统的调度方式。

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
return 0;
}
```

此示例演示了以下内容：

- `component_concept` 将类型标记为 ECS 组件。

- `basic_querior` 可以从世界构建，并使用`with`、`without` 和 `withany` 进行过滤。

- `call_update(world)` 驱动世界的 `pre_update`、`update` 和 `post_update` 阶段。

## 直接世界操作

**通常, 这是不那么推荐的做法, 除非你清楚你在做什么.**

如果您只需要存储层，`world_base` 和 `basic_world<world_descriptor_t<>>` 也支持直接结构编辑：

```cpp
neutron::world_base<> world;
auto entity = world.spawn(Position{1.0f, 2.0f});
world.add_components(entity, Velocity{3.0f, 4.0f});
world.remove_components<Velocity>(entity);
auto batch = world.spawn_n(1024, Position{}, Velocity{});
world.kill(batch);
```

对于延迟结构编辑，请使用 `neutron::command_buffer<>`：

```cpp
neutron::basic_world</* descriptor */> world;
neutron::command_buffer<> commands;
auto future = commands.spawn(Position{1.0f, 2.0f});
commands.add_components(future, Velocity{3.0f, 4.0f}); commands.apply(world);
```

**通常, 不要将裸的世界描述符传入`basic_world`**

直接 API 还包括 `reserve`、`spawn_n`、`is_alive`、`kill` 以及

用于批量工作流程的组件添加/移除操作。

## 描述符、调度和系统输入

世界描述符通过组合小型适配器构建：

- `add_systems<stage, ...>` 为阶段注册系统。

- `execute<group<N>>` 分配分组执行顺序。

- `execute<individual>` 允许世界在其自身的执行路径上运行。

- `execute<frequency<...>>` 和 `execute<dynamic_frequency<>>` 控制更新频率。

- `enable_render` 和 `enable_events` 附加渲染和事件元数据。

系统可以请求的不仅仅是组件查询。 neutron 还支持：

- `basic_commands<>`，用于通过命令缓冲区进行结构编辑

- `res<T...>`，用于管理世界拥有的资源

- `global<T...>`，用于管理外部绑定的共享状态

- `local<T...>`，用于管理每个系统的本地存储

对于多世界执行，运行时辅助函数位于[`<neutron/ecs.hpp>`](../../include/neutron/ecs.hpp) 和
[`<neutron/execution_resources.hpp>`](../../include/neutron/execution_resources.hpp)
涵盖了诸如 `make_worlds`、`call_update(scheduler, command_buffers, worlds)` 和 `make_world_schedule(...)` 等模式。

描述符机制在编译期进行元数据推导, 这能降低运行时开销.

当然, 我更建议你看看[`app.hpp`](https://github.com/Shucharjer/electron/tree/main/include/electron/app/app.hpp) 和[`app_example.cpp`](https://github.com/Shucharjer/electron/tree/main/example/src/main.cpp) 了解它在真实场景中的运行方式. 在app_example中, 使用glfw和vulkan绘制了一个窗口, 这个窗口有一个imgui窗口，上面写着`hello world`.

## 其他公共模块

### `execution`

[`<neutron/execution.hpp>`](../../include/neutron/execution.hpp) 公开了一个发送方/接收方风格的执行层。如果存在标准实现或`stdexec`，neutron 会将其转发；否则，将使用回退实现。


```cpp
#include <neutron/execution.hpp>

auto sndr = neutron::execution::just(40)
| neutron::execution::then([](int value) { return value + 2; });
auto [answer] = neutron::this_thread::sync_wait(std::move(sndr)).value();
```

### 反射

[`<neutron/reflection.hpp>`](../../include/neutron/reflection.hpp) 提供聚合类型的编译时成员检查：

```cpp
#include <neutron/reflection.hpp>

struct Vec2 {
float x;
float y;
};

static_assert(neutron::member_count_of<Vec2>() == 2);
constexpr auto names = neutron::member_names_of<Vec2>();
```

对于非聚合类型，可以使用 `REFL_MEMBERS(Type, ...)` 选择启用。

### 容器和实用工具

一些常用的入口点包括：

- [`<neutron/smvec.hpp>`](../../include/neutron/smvec.hpp): 一个类似向量的容器带有内联存储。

- [`<neutron/sparse_map.hpp>`](../../include/neutron/sparse_map.hpp), 通过空间换取时间的极速关联容器

- [`<neutron/shift_map.hpp>`](../../include/neutron/shift_map.hpp): 针对索引存储进行优化的`sparse_map`

- [`<neutron/flat_hash_map.hpp>`](../../include/neutron/flat_hash_map.hpp), 一个泛用的高速关联容器

- [`<neutron/polymorphic.hpp>`](../../include/neutron/polymorphic.hpp) 用于轻量级类型擦除多态性

- [`<neutron/ranges.hpp>`](../../include/neutron/ranges.hpp) 提供了`ranges::to`, `range_adaptor_closure`以及一些概念

- [`<neutron/memory.hpp>`](../../include/neutron/memory.hpp) 提供了存储感知的`uninitialized_*`等内存相关的工具

- [`<neutron/functional.hpp>`](../../include/neutron/functional.hpp) 提供低开销的函数调度方式

## 构建测试和基准测试

Neutron 可以配置为独立的 CMake 项目：

```bash
cmake -S third_party/neutron -B build/neutron -DBUILD_NEUTRON_TESTING=ON
cmake --build build/neutron
ctest --test-dir build/neutron
```

要启用基准测试，请添加 `-DBUILD_NEUTRON_BENCHMARK=ON`。基准测试构建通过 CMake `FetchContent` 使用 Google Benchmark。

## 许可证

Neutron 根据 MIT 许可证发布。详情请参阅 [LICENSE](LICENSE)。
