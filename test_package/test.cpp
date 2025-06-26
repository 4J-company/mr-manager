#include <string>

#include <gtest/gtest.h>

#include <mr-manager/manager.hpp>

using namespace mr;

class ManagerTest : public ::testing::Test {
protected:
  void TearDown() override {
    // Clear all managers after each test
    Manager<int>::get()._table.clear();
    Manager<std::string>::get()._table.clear();
  }
};

TEST_F(ManagerTest, BasicObjectCreation) {
  auto& manager = Manager<int>::get();
  const std::string id = "test_int";

  auto handle = manager.create(id, 42);
  auto& entry = handle->second;

  ASSERT_NE(entry.ptr, nullptr);
  EXPECT_EQ(*entry.ptr, 42);
}

TEST_F(ManagerTest, MultipleObjectTypes) {
  // Test with int manager
  auto& int_manager = Manager<int>::get();
  auto int_handle = int_manager.create("int_val", 100);
  EXPECT_EQ(*int_handle->second.ptr, 100);

  // Test with string manager
  auto& str_manager = Manager<std::string>::get();
  auto str_handle = str_manager.create("str_val", "hello");
  EXPECT_EQ(*str_handle->second.ptr, "hello");
}

TEST_F(ManagerTest, ObjectUpdate) {
  auto& manager = Manager<int>::get();
  const std::string id = "update_test";

  // First creation
  auto handle1 = manager.create(id, 10);
  EXPECT_EQ(*handle1->second.ptr, 10);

  // Update value
  auto handle2 = manager.create(id, 20);
  EXPECT_EQ(*handle2->second.ptr, 20);

  // Verify only one entry exists
  EXPECT_EQ(manager._table.size(), 1);
}

TEST_F(ManagerTest, HandleValidity) {
  auto& manager = Manager<int>::get();
  const std::string id = "handle_test";

  auto handle = manager.create(id, 30);
  auto& entry = handle->second;

  // Verify handle points to correct value
  EXPECT_EQ(handle->first, id);
  EXPECT_EQ(*entry.ptr, 30);

  // Modify through pointer
  *entry.ptr = 99;
  EXPECT_EQ(*manager._table.find(id)->second.ptr, 99);
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
  auto& entry = handle->second;

  EXPECT_EQ(entry.ptr->a, 1);
  EXPECT_DOUBLE_EQ(entry.ptr->b, 2.5);
  EXPECT_EQ(entry.ptr->c, "test");
}

TEST_F(ManagerTest, MemoryReclamation) {
  auto& manager = Manager<int>::get();
  const std::string id = "memory_test";

  {
    auto handle = manager.create(id, 42);
    EXPECT_EQ(*handle->second.ptr, 42);
  }

  // Memory should be reclaimed when entry is overwritten
  auto new_handle = manager.create(id, 99);
  EXPECT_EQ(*new_handle->second.ptr, 99);
}

TEST_F(ManagerTest, ConcurrencyStressTest) {
  auto& manager = Manager<int>::get();
  const int num_threads = 4;
  const int operations_per_thread = 1000;

  auto worker = [&](int thread_id) {
    for (int i = 0; i < operations_per_thread; ++i) {
      std::string id = "thread_" + std::to_string(thread_id) + "_" + std::to_string(i);
      auto handle = manager.create(id, i);
      EXPECT_EQ(*handle->second.ptr, i);
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
  EXPECT_EQ(manager._table.size(), num_threads * operations_per_thread);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
