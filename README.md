# mr-manager

**mr-manager** is a wait-free, per-type memory pool object manager for C++23, designed for high-performance, concurrent applications. It leverages [folly](https://github.com/facebook/folly) for synchronization and concurrent hash maps, and uses C++ polymorphic memory resources for efficient memory management.

## Features

- **Wait-free** object management
- **Per-type memory pools** using C++17 polymorphic allocators
- **Concurrent access** via folly's RCU and `ConcurrentHashMap`
- **Type-safe handles** for asset access
- **Customizable hashing** for asset IDs

---

## Build Instructions

### Requirements

- C++23 compatible compiler
- [Conan](https://conan.io/) for dependency management
- [CMake](https://cmake.org/) >= 3.26 (handled by Conan)
- [folly](https://github.com/facebook/folly) (handled by Conan)
- [fmt](https://github.com/fmtlib/fmt) (handled by Conan)
- (Linux) [mold](https://github.com/rui314/mold) linker (optional, for faster linking)

### Quick Start

#### 1. Install Conan dependencies

```sh
conan install . --build=missing
```

#### 2. Build using Conan

```sh
conan build .
```

---

## Usage Example

Below is a minimal example of how to use `mr-manager` to manage a custom type:

```cpp
#include <mr-manager/manager.hpp>
#include <iostream>
#include <string>

struct MyAsset {
    std::string name;
    int value;
    MyAsset(std::string n, int v) : name(std::move(n)), value(v) {}
};

int main() {
    // Get the singleton manager for MyAsset
    auto& manager = mr::Manager<MyAsset>::get();

    // Create an asset (the hash of arguments is used as the ID)
    auto handle = manager.create("example", 42);

    // Access the asset using the handle
    handle.with([](const MyAsset& asset) {
        std::cout << "Asset: " << asset.name << ", value: " << asset.value << std::endl;
    });

    return 0;
}
```

### How it works

- `mr::Manager<T>::get()` returns a singleton manager for type `T`.
- `manager.create(args...)` constructs a new asset of type `T` with the given arguments, or returns a handle to an existing one with the same hash.
- `Handle<T>::with(func)` safely accesses the asset, passing it to your function if it exists.

### Custom Hashing

The asset ID is computed using a custom hash function (see `include/mr-manager/hash.hpp`), which combines the hashes of the constructor arguments.

---

## Integration

To use `mr-manager` in your own project:

1. Add it as a Conan dependency in your `conanfile.txt` or `conanfile.py`:

    ```
    [requires]
    mr-manager/1.0
    ```

2. Link against the library in your CMake project:

    ```cmake
    find_package(mr-manager REQUIRED)
    target_link_libraries(your_target PRIVATE mr-manager)
    ```

---

## License

MIT
