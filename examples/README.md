# Examples

Each file is a small, standalone program showing one part of Neutron.

| Example | Shows |
| --- | --- |
| [execution.cpp](execution.cpp) | composing senders with `then`/`when_all` and `sync_wait` |
| [reflection.cpp](reflection.cpp) | member count, names and pointers of a plain aggregate |
| [small_vector.cpp](small_vector.cpp) | inline storage that spills to the heap |
| [inplace_vector.cpp](inplace_vector.cpp) | fixed-capacity vector built at compile time |
| [flat_hash_map.cpp](flat_hash_map.cpp) | open-addressing hash map |
| [sparse_map.cpp](sparse_map.cpp) | id-to-value map with dense iteration |
| [spsc_queue.cpp](spsc_queue.cpp) | lock-free single-producer single-consumer queue |
| [ranges.cpp](ranges.cpp) | `ranges::to` and custom pipeable adaptors |
| [tstring.cpp](tstring.cpp) | string literals as template arguments |
| [compressed_pair.cpp](compressed_pair.cpp) | pair whose empty members take no space |

## Building

```bash
cmake -S . -B build -DBUILD_NEUTRON_EXAMPLES=ON
cmake --build build
./build/examples/examples.reflection
```

`execution.cpp` needs a sender/receiver implementation: configure with
`-DNEUTRON_USES_STDEXEC=ON`, `-DNEUTRON_USES_BEMAN_EXECUTION=ON`, or use a
standard library that satisfy `__cpp_lib_senders >= 202406L`.
