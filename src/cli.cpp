#include "todo/cli.hpp"
#include "todo/task_store.hpp"

#include <CLI/CLI.hpp>
#include <iostream>
#include <iomanip>
#include <cstdlib>

namespace todo {

std::filesystem::path resolve_storage_path() {
    const char* env_path = std::getenv("TODO_TASKS_FILE");
    if (env_path != nullptr && *env_path != '\0') {
        return std::filesystem::path(env_path);
    }

    const char* home = std::getenv("HOME");
    if (home != nullptr && *home != '\0') {
        return std::filesystem::path(home) / ".todo" / "tasks.json";
    }

    return std::filesystem::current_path() / ".todo_tasks.json";
}

int run_cli(int argc, const char* const* argv) {
    CLI::App app{"Command-line to-do manager"};
    app.require_subcommand(1);

    auto storage_path = resolve_storage_path();
    TaskStore store(storage_path);

    // -----------------------------------------------------------------------
    // Command: todo add "<title>" [--priority low|med|high]
    // -----------------------------------------------------------------------
    auto* add_cmd = app.add_subcommand("add", "Add a new task");
    std::string title;
    std::string priority_str = "med";
    add_cmd->add_option("title", title, "Task title")->required();
    add_cmd->add_option("-p,--priority", priority_str, "Priority (low, med, high)")
           ->check(CLI::IsMember({"low", "med", "high"}, CLI::ignore_case));

    add_cmd->callback([&]() {
        auto prio_opt = parse_priority(priority_str);
        Priority prio = prio_opt.value_or(Priority::Med);
        uint32_t id = store.add_task(title, prio);
        if (store.save()) {
            std::cout << "Added task [" << id << "]: " << title
                      << " (Priority: " << to_string(prio) << ")\n";
        } else {
            std::cerr << "Error: Failed to save tasks to " << store.file_path() << "\n";
        }
    });

    // -----------------------------------------------------------------------
    // Command: todo list [--done|--pending]
    // -----------------------------------------------------------------------
    auto* list_cmd = app.add_subcommand("list", "List tasks");
    bool filter_done = false;
    bool filter_pending = false;
    auto* done_flag = list_cmd->add_flag("--done", filter_done, "Show only completed tasks");
    auto* pending_flag = list_cmd->add_flag("--pending", filter_pending, "Show only pending tasks");
    done_flag->excludes(pending_flag);

    list_cmd->callback([&]() {
        const auto& tasks = store.get_all_tasks();
        if (tasks.empty()) {
            std::cout << "No tasks found.\n";
            return;
        }

        std::vector<Task> filtered;
        for (const auto& t : tasks) {
            if (filter_done && !t.done) continue;
            if (filter_pending && t.done) continue;
            filtered.push_back(t);
        }

        if (filtered.empty()) {
            std::cout << "No matching tasks found.\n";
            return;
        }

        std::cout << std::left 
                  << std::setw(6)  << "ID"
                  << std::setw(10) << "STATUS"
                  << std::setw(12) << "PRIORITY"
                  << "TITLE\n";
        std::cout << std::string(50, '-') << "\n";

        for (const auto& t : filtered) {
            std::string status = t.done ? "[DONE]" : "[PENDING]";
            std::cout << std::left
                      << std::setw(6)  << t.id
                      << std::setw(10) << status
                      << std::setw(12) << to_string(t.priority)
                      << t.title << "\n";
        }
    });

    // -----------------------------------------------------------------------
    // Command: todo done <id>
    // -----------------------------------------------------------------------
    auto* done_cmd = app.add_subcommand("done", "Mark a task as completed");
    uint32_t done_id = 0;
    done_cmd->add_option("id", done_id, "Task ID")->required();

    done_cmd->callback([&]() {
        auto updated = store.mark_done(done_id);
        if (!updated.has_value()) {
            std::cerr << "Error: Task with ID " << done_id << " not found.\n";
            return;
        }
        if (store.save()) {
            std::cout << "Marked task [" << done_id << "] as done: "
                      << updated->title << "\n";
        } else {
            std::cerr << "Error: Failed to save changes.\n";
        }
    });

    // -----------------------------------------------------------------------
    // Command: todo remove <id>
    // -----------------------------------------------------------------------
    auto* remove_cmd = app.add_subcommand("remove", "Remove a task by ID");
    uint32_t remove_id = 0;
    remove_cmd->add_option("id", remove_id, "Task ID")->required();

    remove_cmd->callback([&]() {
        bool removed = store.remove_task(remove_id);
        if (!removed) {
            std::cerr << "Error: Task with ID " << remove_id << " not found.\n";
            return;
        }
        if (store.save()) {
            std::cout << "Removed task [" << remove_id << "].\n";
        } else {
            std::cerr << "Error: Failed to save changes.\n";
        }
    });

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    return 0;
}

} // namespace todo
