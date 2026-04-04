# ECS 实现

这一页聚焦 `neutron` 的 ECS 本身，默认读者已经看过 [`readme.md`](./readme.md) 中的快速入门.
与快速入门不同，这里更强调它的实现分层、描述符驱动的调度方式，以及它在 `electron`示例中的行为.

相关入口仍然是 [`<neutron/ecs.hpp>`](../../include/neutron/ecs.hpp).

## 设计目标

`neutron` 的 ECS 围绕三件事组织：

- 用 archetype 布局把同构实体放到连续存储中，让查询和批量结构编辑尽量保持缓存友好;  
- 用世界描述符在编译期推导系统元数据、依赖关系和访问冲突，把错误尽量提前到编译期;  
- 把存储层、延迟结构编辑和多世界调度分开，让调用方可以按成本选择抽象层级.  

因此，这套 ECS 并不只有一个“世界”类型，而是拆成了几层：

- `world_base<>` 提供底层存储和直接结构编辑.
- `basic_world<Descriptor>` 或 `make_world<Descriptor>()` 在存储层之上叠加描述符、系统图和运行接口.
- `make_worlds<...>`、`call_update(...)`、`make_world_schedule(...)`和`make_runtime` 负责多世界运行时.

如果你只是想把实体和组件放进连续内存里，直接使用 `world_base<>` 即可；如果你需要阶段化系统、命令缓冲、查询缓存、频率控制或多世界调度，则使用描述符驱动的 `basic_world` 更合适.

## 数据模型

### 组件

组件是普通 C++ 类型，只要满足 `component_concept` 标记或者特化 `as_component` 即可：

```cpp
struct Position {
    using component_concept = neutron::component_t;
    float x{};
    float y{};
};

struct Velocity {
    float x{};
    float y{};
};
template <>
constexpr bool ::neutron::as_component<Velocity> = true;
```

这意味着 `neutron` 并不强迫你继承某个基类，也不要求运行时注册.组件识别主要依赖类型系统，系统参数和描述符元数据也因此可以在编译期推导。

### 实体与 archetype

实体由 `entity_t` 标识.底层存储不是“每个实体一个对象”，而是把拥有相同组件集合的实体收进同一个archetype。这样做带来两个直接结果：

- 查询某一组组件时，遍历的是少量连续缓冲区，而不是离散对象.
- `add_components` / `remove_components` 的本质是把实体从一个 archetype 迁移到另一个 archetype.

[`test/src/ecs/world_base.cpp`](../../test/src/ecs/world_base.cpp) 和
[`test/src/ecs/archetype.cpp`](../../test/src/ecs/archetype.cpp) 展示了这层语义：

- `spawn(Position{}, Velocity{})` 会把实体放进对应组件集合的 archetype.
- `add_components(entity, Velocity{})` 会触发结构迁移，但已有组件的重复添加会退化为 no-op.
- `remove_components<Tag>(entity)` 会迁移到新的 archetype；删除不存在的组件同样是 no-op.
- `spawn_n(...)`、`kill(...)` 和批量保留容量接口是第一等能力，而不是附加工具函数.

这种设计与以“稀疏索引 + 独立组件池”为核心的 ECS 不同，更偏向于结构稳定后的批量处理.

## 世界分层

### `world_base<>`

`world_base<>` 是最接近存储层的世界类型，适合手写结构编辑或做细粒度控制：

```cpp
neutron::world_base<> world;

auto entity = world.spawn(Position{1.0f, 2.0f});
world.add_components(entity, Velocity{3.0f, 4.0f});
world.remove_components<Velocity>(entity);

auto batch = world.spawn_n(1024, Position{}, Velocity{});
world.kill(batch);
```

这类接口在 [`test/src/ecs/world_base.cpp`](../../test/src/ecs/world_base.cpp) 中覆盖得很完整，
包括：

- 单个实体和批量实体创建.
- 带值和默认构造两种组件添加路径.
- 重复添加、重复移除、先加后删、先删后加等边界条件.
- 实体回收与 `is_alive` 语义.

如果你只需要一个“高性能结构化存储”，这一层已经够用.

### `basic_world<Descriptor>` 与 `make_world<Descriptor>()`

当你引入系统和阶段时，世界类型会由描述符决定：

```cpp
constexpr auto game_world =
    neutron::world_desc |
    neutron::add_systems<neutron::stage::update, &integrate>;

auto world = neutron::make_world<game_world>();
neutron::call_update(world);
```

这个层次除了保留 `spawn`、`add_components` 等底层编辑接口，还会：

- 在编译期生成阶段图和系统调用路径.
- 把系统参数自动从世界中构造出来.
- 在调用系统后自动同步查询快照与命令缓冲.

[`test/src/ecs/ecs.cpp`](../../test/src/ecs/ecs.cpp) 覆盖了单世界和多世界的 `call` /
`call_update` 行为，也验证了 grouped world、individual world、动态频率和 render hooks 的区别.

## 世界描述符与调度

`neutron` 的 ECS 关键不在“系统列表”，而在 `world_desc` 及其后续组合：

```cpp
constexpr auto world =
    world_desc |
    enable_render |
    add_systems<render, &renderHello> |
    add_systems<pre_update, &observeApp> |
    add_systems<pre_update, &printFps>;
```

这段代码直接来自 [`electron/example/src/main.cpp`](https://github.com/Shucharjer/electron/blob/main/example/src/main.cpp)，它同时体现了三件事：

- 世界功能通过描述符声明，而不是靠运行时注册.
- 系统属于明确的阶段，例如 `pre_update` 或 `render`.
- `enable_render` 这样的标记会在编译期写入描述符元数据，并影响运行时调度.

### `add_systems`

`add_systems<stage, ...>` 用于把系统放到某个阶段，并可以在局部写出额外约束：

```cpp
constexpr auto desc =
    world_desc |
    add_systems<
        update,
        { &structural },
        { &fn },
        { &after_structural, after<&structural> }>;
```

这类写法在 [`test/src/ecs/ecs.cpp`](../../test/src/ecs/ecs.cpp)、
[`test/src/ecs/desc.cpp`](../../test/src/ecs/desc.cpp) 和
[`test/src/ecs/graph.cpp`](../../test/src/ecs/graph.cpp) 中都有覆盖.`after<&foo>` / `before<&foo>` 不是运行时排序提示，而是编译期图约束的一部分。

### `execute<...>`

`execute<...>` 用来补充世界级执行策略：

- `execute<group<N>>` 指定世界所属的组，决定多世界运行时的组序.
- `execute<individual>` 让世界走单独的执行路径.
- `execute<frequency<...>>` 指定固定更新频率.
- `execute<dynamic_frequency<>>` 允许在运行时设置更新间隔.
- `execute<always>` 表示总是更新；这也是默认描述符的一部分.

[`test/src/ecs/execute.cpp`](../../test/src/ecs/execute.cpp) 验证了这些规则的归一化结果：

- 默认 `world_desc` 会被归一化成 `group<0> + always`.
- `group` 与 `frequency` 可以在世界级别组合.
- `individual` 会切换执行域.
- `dynamic_frequency<>` 不能与静态 `frequency<...>` 混用.

### 编译期验证

这套描述符不是只负责“记配置”，还会做大量编译期检查.

[`test/src/ecs/metainfo.cpp`](../../test/src/ecs/metainfo.cpp) 展示了几类典型验证：

- 组件读写冲突会被拒绝.
- 资源 `res<T...>` 与全局 `global<T...>` 的读写冲突也会被拒绝.
- 通过 `after<...>` 建立顺序后，原本冲突的系统可以合法化.
- 跨 group 的依赖和环状依赖会被识别为非法描述符.

[`test/src/ecs/graph.cpp`](../../test/src/ecs/graph.cpp) 还说明了运行图如何处理带命令缓冲的节点：

- 先收集当前可运行节点.
- 发现命令节点完成后，标记命令缓冲需要刷新.
- 刷新完成后再释放后继节点.

换句话说，`basic_commands<>` 带来的结构修改不会在任意时刻穿透系统图，而是在flush时完成.

## 查询与系统参数

### `query` 与 `basic_querior`

查询是系统最常见的输入.一个典型系统如下：

```cpp
void integrate(query<with<Position&, Velocity&>> movers) {
    for (auto&& [position, velocity] : movers.get()) {
        position.x += velocity.x;
        position.y += velocity.y;
    }
}
```

其中几个细节很重要：

- `with<T...>`、`without<T...>` 和 `withany<T...>` 负责过滤;  
- `query` 遍历的是 archetype 切片，不需要逐实体做类型分派;  
- 系统被调用时，查询对象会自动从世界构造.  

[`test/src/ecs/query.cpp`](../../test/src/ecs/query.cpp) 还说明了两种不同的查询时机：

- 手动构造 `basic_querior` 时，查询视图保留构造时刻的快照语义.
- 系统参数里的缓存查询会在每次系统调用前同步，因此能看到最新世界状态.

### `basic_commands<>`

如果系统需要做结构编辑，推荐请求 `basic_commands<>`，而不是在遍历过程中直接改世界：

```cpp
void structural(neutron::basic_commands<>) {}
```

这让结构变更进入命令缓冲，等到安全的 flush 点再统一应用.相关行为可见
[`test/src/ecs/ecs.cpp`](../../test/src/ecs/ecs.cpp) 和
[`test/src/ecs/graph.cpp`](../../test/src/ecs/graph.cpp).

### `res<T...>`、`global<T...>`、`local<T...>`、`insertion<T...>`

系统参数不只有查询：

- `res<T...>` 访问世界拥有的资源.
- `global<T...>` 访问外部绑定的共享对象.
- `local<T...>` 访问某个系统自身的局部状态.
- `insertion<T...>` 访问当前调用链路临时注入的引用对象.

[`electron/example/include/fps.hpp`](https://github.com/Shucharjer/electron/blob/main/example/include/fps.hpp) 展示了 `local<T...>` 的典型用法：

```cpp
void printFps(
    neutron::local<
        std::chrono::time_point<std::chrono::system_clock>,
        duration,
        ticks_t> local) {
    auto [tp, dur, ticks] = local;
    // ...
}
```

这个局部状态不需要挂成组件或全局资源，也不需要手写静态变量；它会跟着系统实例存在.

而 [`electron/example/src/main.cpp`](https://github.com/Shucharjer/electron/blob/main/example/src/main.cpp) 中的：

```cpp
void observeApp(neutron::insertion<const App::insertion&> app) {
    [[maybe_unused]] auto& [insertion] = app;
}
```

则说明了 `insertion<T...>` 的定位：它不是长期存储，而是把当前运行上下文中的外部引用安全地送进系统.
这对引擎层桥接窗口、渲染器或应用对象尤其有用.

## 延迟结构编辑

除了系统参数里的 `basic_commands<>`，也可以手动使用 `command_buffer<>`：

```cpp
neutron::world_base<> world;
neutron::command_buffer<> commands;

auto future = commands.spawn(Position{1.0f, 2.0f});
commands.add_components(future, Velocity{3.0f, 4.0f});
commands.apply(world);
```

这里的 `future_entity_t` 允许你在实体真正创建之前继续给它追加结构编辑.
[`test/src/ecs/world_base.cpp`](../../test/src/ecs/world_base.cpp) 已经覆盖了：

- 先 `spawn` 再 `add_components`;  
- 先 `spawn` 再 `remove_components`;  
- 对未来实体的结构命令在 `apply(world)` 后一次性兑现.  

这一层通常适合：

- 需要在离线批处理中组织结构修改;  
- 不想马上把修改应用到世界;  
- 希望把编辑逻辑与系统执行阶段分离.  

但需要说明的是, 如果能预先确定添加哪些组件, 一次性完成实体生成会比再调用add_components拥有更低的开销.  

## 多世界运行时

单世界场景里，`call<stage>(world)` 和 `call_update(world)` 已经足够.  
但在真实引擎里，常见情况是多个世界需要共享调度器、命令缓冲和外部 hooks.  

这时可以使用:  

- `make_worlds<...>()`
- `call<stage>(scheduler, command_buffers, worlds)`
- `call_update(scheduler, command_buffers, worlds)`
- `make_world_schedule(...)`

[`test/src/ecs/ecs.cpp`](../../test/src/ecs/ecs.cpp) 验证了几个关键行为：

- grouped world 和 individual world 会走不同的执行路径.
- group 顺序是确定性的，可以手动 `call_update_group<N>`.
- render world 在 schedule hooks 存在时会围绕一帧触发 `render_begin` / `render_end`.
- `global<T...>` 可以跨世界共享外部状态.

这也是 `electron` 示例能把应用生命周期、渲染阶段和 ECS 系统拼到一起的原因.

## `electron/example` 如何使用这套 ECS

[`electron/example/src/main.cpp`](https://github.com/Shucharjer/electron/blob/main/example/src/main.cpp) 是理解这套 ECS 最直接的真实例子：

```cpp
constexpr auto world = world_desc | enable_render |
                       add_systems<render, &renderHello> |
                       add_systems<pre_update, &observeApp> |
                       add_systems<pre_update, &printFps>;

App::Create() | run_worlds<world>(config);
```

这个例子对应了 ECS 的三个典型角色：

- [`example/include/hello_world.h2`](https://github.com/Shucharjer/electron/blob/main/example/include/hello_world.h2)中的 `renderHello` 是渲染阶段系统，说明系统本身可以只是一个普通函数.
- [`example/include/fps.hpp`](https://github.com/Shucharjer/electron/blob/main/example/include/fps.hpp)中的 `printFps` 使用 `local<T...>` 持有跨帧局部状态.
- [`example/src/main.cpp`](https://github.com/Shucharjer/electron/blob/main/example/src/main.cpp)中的 `observeApp` 使用 `insertion<const App::insertion&>` 接入外部应用上下文.

因此，`neutron` 的 ECS 并不是“自己包办一个完整游戏框架”，而是把系统组织、查询和调度能力嵌到
`electron` 的应用外壳里.引擎壳层负责窗口、渲染器、事件循环和平台对象，ECS 负责把这些能力变成可组合的系统流水线。

## Benchmark 结果图

下面这些图位于 [`../image`](../image)，在本文中仅作为参考引用.

需要特别说明几件事：

- benchmark结果是在 MIT协议的开源仓库 [abeimler/ecs_benchmark](https://github.com/abeimler/ecs_benchmark) 的基础上进行更改得到的, 并非本仓库benchmark目录下的得到的结果;  
- 所用 `neutron` 版本为 `bfa6c9e0cd008d53bb96e02e654a307eff2860e5`, 并非最新版本 (现在加入了查询缓存 `slice`, 应该会在更新上表现更佳);  
- `ecs_benchmark`缺乏及时维护, benchmark 代码由 AI 完成，结果仅供参考.

### 实体创建与结构变更

空实体创建：

![Create empty entities](../image/CreateEmptyEntities.svg)

批量创建空实体：

![Create empty entities in bulk](../image/CreateEmptyEntitiesInBulk.svg)

创建带组件实体：

![Create entities](../image/CreateEntities.svg)

批量创建带组件实体：

![Create entities in bulk](../image/CreateEntitiesInBulk.svg)

向实体添加组件：

![Add component](../image/AddComponent.svg)

对实体先移除再添加组件：

![Remove and add component](../image/RemoveAddComponent.svg)

### 查询与遍历

遍历单组件实体：

![Iterate single component](../image/IterateSingleComponent.svg)

遍历双组件实体：

![Iterate two components](../image/IterateTwoComponents.svg)

遍历混合实体中的三组件集合：

![Iterate three components with mixed entities](../image/IterateThreeComponentsWithMixedEntities.svg)

### 实体销毁

逐个销毁实体：

![Destroy entities](../image/DestroyEntities.svg)

批量销毁实体：

![Destroy entities in bulk](../image/DestroyEntitiesInBulk.svg)

### 系统更新

两系统更新：

![Systems update](../image/SystemsUpdate.svg)

两系统更新，实体组件混合：

![Systems update mixed entities](../image/SystemsUpdateMixedEntities.svg)

七系统更新：

![Complex systems update](../image/ComplexSystemsUpdate.svg)

七系统更新，实体组件混合：

![Complex systems update mixed entities](../image/ComplexSystemsUpdateMixedEntities.svg)

这些图更适合帮助你观察趋势，而不是给出某个绝对结论.若你要分析 `neutron` 的真实表现，最好结合自己的实际系统数量、组件大小和结构变化密度进行判断.
