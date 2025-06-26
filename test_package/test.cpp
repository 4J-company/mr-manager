#include <string>
#include <atomic>
#include <thread>
#include <vector>
#include <random>
#include <barrier>
#include <latch>

#include <gtest/gtest.h>

#include <mr-manager/manager.hpp>

using namespace mr;

class ManagerTest : public ::testing::Test {
protected:
  void TearDown() override {
    // Clear all managers after each test
    Manager<int>::get().clear();
    Manager<std::string>::get().clear();
  }
};

TEST_F(ManagerTest, BasicObjectCreation) {
  auto& manager = Manager<int>::get();
  const std::string id = "test_int";

  auto handle = manager.create(id, 42);

  ASSERT_EQ(handle.value(), 42);
}

TEST_F(ManagerTest, MultipleObjectTypes) {
  // Test with int manager
  auto& int_manager = Manager<int>::get();
  auto int_handle = int_manager.create("int_val", 100);
  EXPECT_EQ(int_handle.value(), 100);

  // Test with string manager
  auto& str_manager = Manager<std::string>::get();
  auto str_handle = str_manager.create("str_val", "hello");
  EXPECT_EQ(str_handle.value(), "hello");
}

TEST_F(ManagerTest, ObjectUpdate) {
  auto& manager = Manager<int>::get();
  const std::string id = "update_test";

  // First creation
  auto handle1 = manager.create(id, 10);
  EXPECT_EQ(handle1.value(), 10);

  // Update value
  auto handle2 = manager.create(id, 20);
  EXPECT_EQ(handle2.value(), 20);

  // Verify only one entry exists
  EXPECT_EQ(manager.size(), 1);
}

TEST_F(ManagerTest, HandleValidity) {
  auto& manager = Manager<int>::get();
  const std::string id = "handle_test";

  auto handle = manager.create(id, 30);

  // Verify handle points to correct value
  EXPECT_EQ(handle.value(), 30);

  handle.value() = 99;
  EXPECT_EQ(manager.find(id)->value(), 99);
}

TEST_F(ManagerTest, CustomTypeManagement) {
  struct TestStruct {
    int a;
    double b;
    std::string c;

    TestStruct(int a, double b, std::string c) : a(a), b(b), c(c) {}
  };

  auto& manager = Manager<TestStruct>::get();
  const std::string id = "struct_test";

  auto handle = manager.create(id, 1, 2.5, "test");

  EXPECT_EQ(handle->a, 1);
  EXPECT_DOUBLE_EQ(handle->b, 2.5);
  EXPECT_EQ(handle->c, "test");
}

TEST_F(ManagerTest, MemoryReclamation) {
  auto& manager = Manager<int>::get();
  const std::string id = "memory_test";

  {
    auto handle = manager.create(id, 42);
    EXPECT_EQ(handle.value(), 42);
  }

  // Memory should be reclaimed when entry is overwritten
  auto new_handle = manager.create(id, 99);
  EXPECT_EQ(new_handle.value(), 99);
}

TEST_F(ManagerTest, ConcurrencyStressTest) {
  auto& manager = Manager<int>::get();
  const int num_threads = 4;
  const int operations_per_thread = 1000;

  auto worker = [&](int thread_id) {
    for (int i = 0; i < operations_per_thread; ++i) {
      std::string id = "thread_" + std::to_string(thread_id) + "_" + std::to_string(i);
      auto handle = manager.create(id, i);
      EXPECT_EQ(handle.value(), i);
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker, i);
  }

  for (auto& t : threads) {
    t.join();
  }

  // Verify all entries were created
  EXPECT_EQ(manager.size(), num_threads * operations_per_thread);
}

using namespace mr;

class ConcurrentReadHeavyTest : public ::testing::Test {
protected:
  void TearDown() override {
    Manager<int>::get().clear();
    Manager<std::string>::get().clear();
  }
};

// Test high-frequency reads with rare writes
TEST_F(ConcurrentReadHeavyTest, RareWritesHighReads) {
  auto& manager = Manager<int>::get();
  const std::string asset_id = "high_value_asset";

  // Initial value
  manager.create(asset_id, 100);

  constexpr int total_readers = 32;
  constexpr int read_iterations = 100000;
  constexpr int write_iterations = 50;  // Rare writes
  std::atomic<int> write_count{0};
  std::atomic<int> read_count{0};
  std::atomic<bool> writers_done{false};

  std::barrier sync_point{total_readers + 1};  // +1 for writer

  // Writer thread (rare updates)
  auto writer = [&] {
    for (int i = 0; i < write_iterations; ++i) {
      // Simulate processing time between writes
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      // Perform atomic update
      manager.create(asset_id, 100 + i);
      write_count++;
    }
    writers_done = true;
  };

  // Reader threads (high frequency)
  auto reader = [&](int thread_id) {
    std::mt19937 rng(thread_id);
    std::uniform_int_distribution<int> dist(1, 5);

    // Get persistent handle (simulates long-lived reference)
    auto handle_opt = manager.find(asset_id);
    ASSERT_TRUE(handle_opt);

    sync_point.arrive_and_wait();

    while (!writers_done) {
      // Read using the persistent handle
      int value = handle_opt->value();

      // Validate value is in expected range
      EXPECT_GE(value, 100);
      EXPECT_LE(value, 100 + write_iterations - 1);

      // Simulate work
      std::this_thread::sleep_for(std::chrono::nanoseconds(dist(rng)));
      read_count++;
    }
  };

  // Start threads
  std::vector<std::thread> readers;
  for (int i = 0; i < total_readers; ++i) {
    readers.emplace_back(reader, i);
  }

  std::thread writer_thread(writer);
  sync_point.arrive_and_wait();  // Synchronize start

  // Wait for writer to finish
  writer_thread.join();

  // Signal readers to exit
  writers_done = true;

  // Wait for readers
  for (auto& t : readers) {
    t.join();
  }

  std::cout << "\nPerformance stats:\n";
  std::cout << "  Reads completed: " << read_count << "\n";
  std::cout << "  Writes completed: " << write_count << "\n";
  std::cout << "  Read/Write ratio: "
    << static_cast<double>(read_count) / write_count << ":1\n";

  // Verify final state
  EXPECT_EQ(manager.find(asset_id)->value(), 100 + write_iterations - 1);
}

// Test handle stability across writes
TEST_F(ConcurrentReadHeavyTest, HandleStabilityDuringWrites) {
  auto& manager = Manager<int>::get();
  const std::string asset_id = "stable_handle_asset";

  manager.create(asset_id, 0);
  auto persistent_handle = manager.find(asset_id);

  constexpr int writers = 2;
  constexpr int write_operations = 100;
  std::latch start_latch{writers};

  auto writer = [&](int thread_id) {
    start_latch.arrive_and_wait();

    for (int i = 0; i < write_operations; ++i) {
      manager.create(asset_id, i);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  };

  std::vector<std::thread> writer_threads;
  for (int i = 0; i < writers; ++i) {
    writer_threads.emplace_back(writer, i);
  }

  // Continuously read via persistent handle
  int last_value = -1;
  int changes_detected = 0;
  for (int i = 0; i < 1000; ++i) {
    int current_value = persistent_handle->value();

    if (current_value != last_value) {
      changes_detected++;
      last_value = current_value;

      // Validate handle still works
      EXPECT_GE(current_value, 0);
      EXPECT_LT(current_value, write_operations);
    }

    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }

  for (auto& t : writer_threads) {
    t.join();
  }

  std::cout << "\nHandle stability stats:\n";
  std::cout << "  Value changes detected: " << changes_detected << "\n";
  EXPECT_GT(changes_detected, 0);
}

// Test read consistency during writes
TEST_F(ConcurrentReadHeavyTest, ReadConsistency) {
  struct ConsistentData {
    int a;
    int b;
    bool consistent;

    ConsistentData(int x, int y) : a(x), b(y), consistent(a * 2 == b) {}
  };

  auto& manager = Manager<ConsistentData>::get();
  const std::string asset_id = "consistent_asset";

  manager.create(asset_id, 0, 0);

  constexpr int readers = 16;
  constexpr int writers = 2;
  constexpr int test_duration_ms = 5000;
  std::atomic<bool> stop{false};
  std::atomic<int> inconsistent_reads{0};
  std::atomic<int> total_reads{0};

  std::barrier sync_point{readers + writers};

  // Writer threads
  auto writer = [&] {
    int value = 0;
    sync_point.arrive_and_wait();

    while (!stop) {
      value++;
      manager.create(asset_id, value, value * 2);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  };

  // Reader threads
  auto reader = [&] {
    sync_point.arrive_and_wait();

    while (!stop) {
      auto handle_opt = manager.find(asset_id);
      if (handle_opt) {
        const auto& data = handle_opt->value();
        total_reads++;

        // Verify internal consistency
        if (!data.consistent) {
          inconsistent_reads++;
          // This should never happen - fail immediately if it does
          ADD_FAILURE() << "Inconsistent state detected: "
            << data.a << ", " << data.b;
        }
      }
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < writers; ++i) {
    threads.emplace_back(writer);
  }
  for (int i = 0; i < readers; ++i) {
    threads.emplace_back(reader);
  }

  // Run test for duration
  std::this_thread::sleep_for(std::chrono::milliseconds(test_duration_ms));
  stop = true;

  for (auto& t : threads) {
    t.join();
  }

  std::cout << "\nConsistency stats:\n";
  std::cout << "  Total reads: " << total_reads << "\n";
  std::cout << "  Inconsistent reads: " << inconsistent_reads << "\n";

  EXPECT_EQ(inconsistent_reads, 0);
}

// Test memory safety under heavy concurrency
TEST_F(ConcurrentReadHeavyTest, MemorySafetyStressTest) {
  auto& manager = Manager<int>::get();
  constexpr int num_assets = 100;
  constexpr int readers_per_asset = 4;
  constexpr int writers_per_asset = 1;
  constexpr int test_duration_ms = 3000;

  std::vector<std::string> asset_ids;
  for (int i = 0; i < num_assets; ++i) {
    asset_ids.push_back("asset_" + std::to_string(i));
    manager.create(asset_ids.back(), i);
  }

  std::atomic<bool> stop{false};
  std::atomic<int> total_reads{0};
  std::atomic<int> total_writes{0};

  auto worker = [&](int thread_id) {
    std::mt19937 rng(thread_id);
    std::uniform_int_distribution<int> asset_dist(0, num_assets - 1);

    while (!stop) {
      int asset_index = asset_dist(rng);
      auto& asset_id = asset_ids[asset_index];

      // 95% reads, 5% writes
      if (rng() % 100 < 5) {
        // Write operation
        int new_value = manager.find(asset_id)->value() + 1;
        manager.create(asset_id, new_value);
        total_writes++;
      } else {
        // Read operation
        auto handle_opt = manager.find(asset_id);
        if (handle_opt) {
          int value = handle_opt->value();
          EXPECT_GE(value, asset_index);  // Should be >= initial value
          total_reads++;
        }
      }

      // Add slight delay to vary timing
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
  };

  constexpr int total_workers = num_assets * (readers_per_asset + writers_per_asset);
  std::vector<std::thread> workers;
  for (int i = 0; i < total_workers; ++i) {
    workers.emplace_back(worker, i);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(test_duration_ms));
  stop = true;

  for (auto& t : workers) {
    t.join();
  }

  std::cout << "\nMemory safety stats:\n";
  std::cout << "  Total reads: " << total_reads << "\n";
  std::cout << "  Total writes: " << total_writes << "\n";
  std::cout << "  Read/Write ratio: "
    << static_cast<double>(total_reads) / total_writes << ":1\n";

  // Verify all assets exist and have been modified
  for (int i = 0; i < num_assets; ++i) {
    auto handle = manager.find(asset_ids[i]);
    EXPECT_TRUE(handle);
    EXPECT_GE(handle->value(), i);
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
