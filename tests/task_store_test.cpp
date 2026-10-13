#include <gtest/gtest.h>
#include "todo/task_store.hpp"
#include <filesystem>
#include <fstream>

namespace todo {
namespace {

class TaskStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a unique temporary file path for isolated test runs
        test_dir_ = std::filesystem::temp_directory_path() / "todo_tests";
        std::filesystem::create_directories(test_dir_);
        test_file_ = test_dir_ / ("test_tasks_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".json");
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(test_dir_, ec);
    }

    std::filesystem::path test_dir_;
    std::filesystem::path test_file_;
};

TEST_F(TaskStoreTest, EmptyStoreReturnsNoTasks) {
    TaskStore store(test_file_);
    EXPECT_TRUE(store.get_all_tasks().empty());
}

TEST_F(TaskStoreTest, AddTaskIncrementsIdAndPersistsInMemory) {
    TaskStore store(test_file_);
    uint32_t id1 = store.add_task("First task", Priority::Low);
    uint32_t id2 = store.add_task("Second task", Priority::High);

    EXPECT_EQ(id1, 1);
    EXPECT_EQ(id2, 2);

    const auto& tasks = store.get_all_tasks();
    ASSERT_EQ(tasks.size(), 2);
    EXPECT_EQ(tasks[0].title, "First task");
    EXPECT_EQ(tasks[0].priority, Priority::Low);
    EXPECT_FALSE(tasks[0].done);

    EXPECT_EQ(tasks[1].title, "Second task");
    EXPECT_EQ(tasks[1].priority, Priority::High);
}

TEST_F(TaskStoreTest, MarkDoneValidAndInvalidId) {
    TaskStore store(test_file_);
    uint32_t id = store.add_task("Write unit tests", Priority::High);

    // Marking nonexistent id returns nullopt
    auto missing = store.mark_done(999);
    EXPECT_FALSE(missing.has_value());

    // Marking valid id updates status and returns updated task
    auto updated = store.mark_done(id);
    ASSERT_TRUE(updated.has_value());
    EXPECT_TRUE(updated->done);
    EXPECT_EQ(updated->id, id);

    auto retrieved = store.get_task(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_TRUE(retrieved->done);
}

TEST_F(TaskStoreTest, RemoveTaskValidAndInvalidId) {
    TaskStore store(test_file_);
    uint32_t id1 = store.add_task("Task 1", Priority::Med);
    uint32_t id2 = store.add_task("Task 2", Priority::Med);

    EXPECT_FALSE(store.remove_task(999));
    EXPECT_EQ(store.get_all_tasks().size(), 2);

    EXPECT_TRUE(store.remove_task(id1));
    EXPECT_EQ(store.get_all_tasks().size(), 1);
    EXPECT_FALSE(store.get_task(id1).has_value());
    EXPECT_TRUE(store.get_task(id2).has_value());
}

TEST_F(TaskStoreTest, PersistenceRoundTripToFile) {
    {
        TaskStore store(test_file_);
        store.add_task("Persisted task 1", Priority::Low);
        uint32_t id2 = store.add_task("Persisted task 2", Priority::High);
        store.mark_done(id2);
        EXPECT_TRUE(store.save());
    }

    // Load in a fresh store instance from the same file
    {
        TaskStore reloaded(test_file_);
        const auto& tasks = reloaded.get_all_tasks();
        ASSERT_EQ(tasks.size(), 2);
        EXPECT_EQ(tasks[0].id, 1);
        EXPECT_EQ(tasks[0].title, "Persisted task 1");
        EXPECT_EQ(tasks[0].priority, Priority::Low);
        EXPECT_FALSE(tasks[0].done);

        EXPECT_EQ(tasks[1].id, 2);
        EXPECT_EQ(tasks[1].title, "Persisted task 2");
        EXPECT_EQ(tasks[1].priority, Priority::High);
        EXPECT_TRUE(tasks[1].done);

        // Next added task should get id 3
        uint32_t id3 = reloaded.add_task("New task after reload", Priority::Med);
        EXPECT_EQ(id3, 3);
    }
}

TEST_F(TaskStoreTest, CorruptedFileLoadsEmptyStoreGracefully) {
    // Write invalid JSON to file
    {
        std::ofstream out(test_file_);
        out << "{ not valid json ...";
    }

    TaskStore store(test_file_);
    EXPECT_TRUE(store.get_all_tasks().empty());
}

} // namespace
} // namespace todo
