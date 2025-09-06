#pragma once

#include <memory_resource>
#include <cassert>
#include <atomic>
#include <string>

#include <folly/concurrency/ConcurrentHashMap.h>

namespace mr {
struct UnnamedTag {};
constexpr inline UnnamedTag unnamed;

template <typename> struct AssetId { using type = std::string; };
template <typename T> using asset_id_t = typename AssetId<T>::type;

template <typename T> struct Manager;

template <typename T>
struct Manager {
  static inline constexpr size_t _max_elements = 1024;
  static inline constexpr size_t _buffer_size = sizeof(T) * _max_elements;

  static inline std::array<std::byte, _buffer_size> _memory_buffer {};
  static inline std::pmr::monotonic_buffer_resource _memory_resource {_memory_buffer.data(), _buffer_size};
  static inline std::pmr::synchronized_pool_resource _memory_pool_resource {&_memory_resource};
  static inline std::pmr::polymorphic_allocator<T> _allocator {&_memory_pool_resource};

  struct Entry {
    T* ptr = nullptr;

    template <typename ...Ts>
    Entry(Ts && ...args) noexcept
    : ptr(_allocator.allocate(1))
    {
      assert(ptr != nullptr);
      new (ptr) T(std::forward<Ts>(args)...);
    }
    ~Entry() noexcept {
      ptr->~T();
      Manager<T>::_allocator.deallocate(ptr, 1);
    }

    Entry() = default;
    Entry(const Entry &other) noexcept = delete;
    Entry & operator=(const Entry &other) noexcept = delete;
    Entry(Entry &&other) noexcept = default;
    Entry & operator=(Entry &&other) noexcept = default;
  };

  using AssetIdT = asset_id_t<T>;
  using HashMapT = folly::ConcurrentHashMap<AssetIdT, Entry>;

  struct Handle {
    T* operator->() noexcept {
      return it->second.ptr;
    }

    const T& value() const noexcept {
      return *it->second.ptr;
    }

    T& value() noexcept {
      return *it->second.ptr;
    }

    HashMapT::const_iterator it;
  };

  static constexpr Manager& get() {
    static Manager instance;
    return instance;
  }

  template<typename ...Args>
  constexpr Handle create(const AssetIdT &id, Args&& ...args) noexcept {
    _table.insert_or_assign(id, T{std::forward<Args>(args)...});
    return { _table.find(id) };
  }

  template<typename ...Args>
  constexpr Handle create(UnnamedTag, Args&& ...args) noexcept {
    static std::atomic_int cnt = 0;
    int id = cnt++;
    std::string idstr = std::to_string(id);
    _table.insert_or_assign(idstr, T{std::forward<Args>(args)...});
    return { _table.find(idstr) };
  }

  constexpr std::optional<Handle> find(const AssetIdT &id) const noexcept {
    auto it = _table.find(id);
    if (it == _table.end()) [[unlikely]] {
      return std::nullopt;
    }
    return Handle{std::move(it)};
  }

  constexpr void clear() noexcept {
    _table.clear();
  }

  constexpr size_t size() const noexcept {
    return _table.size();
  }

private:
  constexpr Manager() noexcept = default;
  constexpr ~Manager() noexcept = default;

  HashMapT _table {_max_elements};
};

} // namespace mr
