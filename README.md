# mr-manager

**mr-manager** is a wait-free, per-type memory pool object manager for C++23, designed for high-performance, concurrent applications. It leverages [folly](https://github.com/facebook/folly) for synchronization and concurrent hash maps, and uses C++ polymorphic memory resources for efficient memory management.

## Features

- **Wait-free** object management
- **Per-type memory pools** using C++17 polymorphic allocators
- **Concurrent access** via folly's `ConcurrentHashMap`
- **Type-safe handles** for asset access

---

## Build Instructions

### Requirements

- C++23 compatible compiler
- [Conan](https://conan.io/) for dependency management
- [CMake](https://cmake.org/) >= 3.26 (handled by Conan)
- [folly](https://github.com/facebook/folly) (handled by Conan)
- [fmt](https://github.com/fmtlib/fmt) (handled by Conan)
- (Linux, optional) [mold](https://github.com/rui314/mold) linker (handled by Conan)

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
    std::string str;
    int num;
    MyAsset(std::string n, int v) : name(std::move(n)), value(v) {}
};

int main() {
    // Get the singleton manager for MyAsset
    auto& manager = mr::Manager<MyAsset>::get();

    // Create an asset (the hash of arguments is used as the ID)
    auto handle = manager.create("custom asset id", "forty two", 42);

    // Access the asset using the handle
    std::println("String: {}; Number: {}", handle->str, handle->num);
}
```

### How it works

- `mr::Manager<T>::get()` returns a singleton manager for type `T`.
- `manager.create(id, args...)` constructs a new asset of type `T` with the given arguments, or returns a handle to an existing one with the same id.
- `Handle<T>::operator->` safely accesses the asset

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
    target_link_libraries(your_target PRIVATE mr-manager::mr-manager)
    ```

---

## License

MIT
