#pragma once

#include <filesystem>
#include <vector>
#include <string>

namespace todo {

// Resolves default ~/.todo/tasks.json or overrides via TODO_TASKS_FILE env var
[[nodiscard]] std::filesystem::path resolve_storage_path();

// Runs the CLI app with standard argc/argv
int run_cli(int argc, const char* const* argv);

} // namespace todo
