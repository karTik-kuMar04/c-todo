#include "todo/task_store.hpp"
#include <fstream>
#include <algorithm>

namespace todo {

TaskStore::TaskStore(std::filesystem::path storage_path)
    : storage_path_(std::move(storage_path)) {
    load();
}

uint32_t TaskStore::add_task(std::string title, Priority priority) {
    uint32_t id = next_id_++;
    tasks_.push_back(Task{
        .id = id,
        .title = std::move(title),
        .priority = priority,
        .done = false
    });
    return id;
}

std::optional<Task> TaskStore::mark_done(uint32_t id) {
    auto it = std::find_if(tasks_.begin(), tasks_.end(), [id](const Task& t) {
        return t.id == id;
    });

    if (it != tasks_.end()) {
        it->done = true;
        return *it;
    }
    return std::nullopt;
}

bool TaskStore::remove_task(uint32_t id) {
    auto it = std::find_if(tasks_.begin(), tasks_.end(), [id](const Task& t) {
        return t.id == id;
    });

    if (it != tasks_.end()) {
        tasks_.erase(it);
        return true;
    }
    return false;
}

const std::vector<Task>& TaskStore::get_all_tasks() const noexcept {
    return tasks_;
}

std::optional<Task> TaskStore::get_task(uint32_t id) const {
    auto it = std::find_if(tasks_.begin(), tasks_.end(), [id](const Task& t) {
        return t.id == id;
    });

    if (it != tasks_.end()) {
        return *it;
    }
    return std::nullopt;
}

void TaskStore::recalculate_next_id() {
    uint32_t max_id = 0;
    for (const auto& t : tasks_) {
        if (t.id > max_id) {
            max_id = t.id;
        }
    }
    next_id_ = max_id + 1;
}

bool TaskStore::load() {
    if (!std::filesystem::exists(storage_path_)) {
        tasks_.clear();
        next_id_ = 1;
        return true;
    }

    std::ifstream file(storage_path_);
    if (!file.is_open()) {
        return false;
    }

    try {
        nlohmann::json j;
        file >> j;
        tasks_ = j.get<std::vector<Task>>();
        recalculate_next_id();
        return true;
    } catch (...) {
        return false;
    }
}

bool TaskStore::save() {
    if (storage_path_.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(storage_path_.parent_path(), ec);
        if (ec) {
            return false;
        }
    }

    std::ofstream file(storage_path_, std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    try {
        nlohmann::json j = tasks_;
        file << j.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace todo
