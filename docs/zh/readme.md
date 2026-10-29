# Neutron

语言: [English](../../README.md) | 简体中文

`neutron` 是一个仅包含头文件的 C++20 实用工具库，面向数据驱动系统和底层运行时场景。

`neutron` 提供 ranges、容器、内存管理、函数工具、编译期反射、sender/receiver 执行模型和基于原型的 ECS。

## 亮点

- 仅包含头文件，易于嵌入到现有的 CMake 项目中。

- 以 C++20 为基线，并在可用时利用更新的标准能力：

  - 在标准库支持的前提下，接口尽量从 C++20 起就能用于常量表达式；

  - 补齐 `ranges::to`、`inplace_vector`、`views::keys`、`views::values` 和 `get` 等标准库接口，以解决性能、语法或兼容性问题；

  - `uninitialized_*` 等原始内存算法以尽可能低的开销提供 C++20 constexpr 支持。

- 面向性能的实现：

  - 通过编译期元数据减少运行时分派；

  - 部分组件针对 C++20/23/26 提供不同实现，优先使用编译器提供的能力；

  - ECS 组件存储采用缓存友好的结构体数组 (SoA) 布局；

  - 容器支持自定义分配器，并为部分类型提供 PMR 别名。

- 聚合类型可直接获得编译期反射；非聚合类型可通过 `REFL_MEMBERS` 启用，编译器原生反射可用时还会自动使用额外能力。

- 提供 `small_vector`、`shift_map`、`sparse_map`、`flat_hash_map` 和 `object_pool` 等容器与存储工具。

- 提供基于原型的 ECS，用于实体/组件存储、查询、分阶段系统和 sender 调度。

## 要求

- C++20 编译器

- CMake 3.21 或更高版本

- execution 后端。除非编译器和标准库已经提供所需 `<execution>` 能力，否则需要按下文选择 `stdexec` 或 `beman::execution`。

## 集成 Neutron

请选择一个 execution 后端。`stdexec` 支持 C++20；`beman::execution` 至少要求 C++23。

使用 [stdexec](https://github.com/NVIDIA/stdexec.git)：

```cmake
set(NEUTRON_USES_STDEXEC ON)
```

或使用 [beman::execution](https://github.com/bemanproject/execution)：

```cmake
set(NEUTRON_USES_BEMAN_EXECUTION ON)
```

最简单的集成方法是添加目录并链接接口目标：

```cmake
set(NEUTRON_USES_STDEXEC ON)
add_subdirectory(${NEUTRON_ROOT_DIR})

target_link_libraries(my_target PRIVATE neutron)
```

neutron 公开了 `neutron` 接口目标和 `atom::neutron` 别名。

由于该库仅包含头文件，因此链接主要传播包含目录、所选 execution 后端和必需的编译选项。两个后端选项互斥，不应同时启用。

## 快速示例

下面的例子直接使用 Neutron 的 ranges 和容器模块：

```cpp
#include <map>
#include <neutron/inplace_vector.hpp>
#include <neutron/ranges.hpp>

int main() {
    neutron::inplace_vector<int, 8> values{1, 2, 3};

    std::map<int, int> lookup{{1, 10}, {2, 20}};
    for (int key : lookup | neutron::views::keys) {
        values.push_back(key);
    }
}
```

## 公共模块

- [ecs](ecs.md)
- 其它公共模块
  - [algorithm](algorithm.md)
  - containers
    - [small_vector](small_vector.md)
    - [inplace_vector](inplace_vector.md)
    - [flat_hash_map](flat_hash_map.md)
    - [bytell_hash_map](bytell_hash_map.md)
    - [sparse_map](sparse_map.md)
    - [shift_map](shift_map.md)
  - [concepts](concepts.md)
  - [execution_resource](execution_resource.md)
  - [functional](functional.md)
  - [interprocess](interprocess.md)
  - [iterator](iterator.md)
  - [lockfree](lockfree.md)
  - [memory](memory.md)
  - [metafn](metafn.md)
  - [poly](poly.md)
  - [ranges](ranges.md)
  - [reflection](reflection.md)
  - [string](string.md)
  - [tuple](tuple.md)
  - [type_traits](type_traits.md)
  - [utility](utility.md)


## 构建测试和基准测试

Neutron 可以配置为独立的 CMake 项目：

```bash
cmake -S . -B build/neutron \
  -DBUILD_NEUTRON_TESTING=ON \
  -DNEUTRON_USES_STDEXEC=ON
cmake --build build/neutron
ctest --test-dir build/neutron
```

要启用基准测试，请添加 `-DBUILD_NEUTRON_BENCHMARK=ON`。基准测试构建通过 CMake `FetchContent` 使用 Google Benchmark。

## 许可证

Neutron 根据 MIT 许可证发布。详情请参阅 [`LICENSE`](../../LICENSE)。
