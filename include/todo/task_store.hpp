#pragma once

#include "todo/task.hpp"

#include <filesystem>
#include <vector>
#include <optional>
#include <cstdint>

namespace todo {

class TaskStore {
public:
    // Explicit constructor: takes a filesystem path to support production
    // persistence (~/.todo/tasks.json) as well as temporary test files.
    explicit TaskStore(std::filesystem::path storage_path);

    // Core domain operations
    // Expected results / failures use std::optional rather than throwing exceptions
    uint32_t add_task(std::string title, Priority priority);
    
    [[nodiscard]] std::optional<Task> mark_done(uint32_t id);
    [[nodiscard]] bool remove_task(uint32_t id);
    
    [[nodiscard]] const std::vector<Task>& get_all_tasks() const noexcept;
    [[nodiscard]] std::optional<Task> get_task(uint32_t id) const;

    // Persistence operations
    bool save();
    bool load();

    [[nodiscard]] const std::filesystem::path& file_path() const noexcept {
        return storage_path_;
    }

private:
    std::filesystem::path storage_path_;
    std::vector<Task> tasks_{};
    uint32_t next_id_{1};

    void recalculate_next_id();
};

} // namespace todo
