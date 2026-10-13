# todo: Modern C++20 Command-Line Task Manager

A clean, production-grade, multi-file C++20 project demonstrating modern build systems, architectural layering, testability, and RAII principles.

---

## 1. Project Directory Structure

```
todo/
├── CMakeLists.txt                 # Build specification, FetchContent, and target graphs
├── .clang-format                  # LLVM-based code formatting standards
├── .gitignore                     # Ignores build artifacts, caches, and editor state
├── Dockerfile                     # Multi-stage container build (lean runtime image)
├── README.md                      # Architecture guide, build manual, and tutorials
├── .github/
│   └── workflows/
│       └── ci.yml                 # Matrix CI testing on GCC 13 and Clang 16
├── include/
│   └── todo/                      # Public interface headers
│       ├── task.hpp               # Domain entity (Task model & Priority enum)
│       ├── task_store.hpp         # Repository (in-memory storage & JSON I/O)
│       └── cli.hpp                # Application CLI command dispatcher
├── src/                           # Implementation files
│   ├── task.cpp                   # Serialization & priority parsing implementation
│   ├── task_store.cpp             # Task storage, query, mutation & persistence logic
│   ├── cli.cpp                    # CLI11 subcommand mapping and view formatting
│   └── main.cpp                   # Executable entrypoint (< 20 lines)
└── tests/                         # Automated unit & integration tests
    ├── task_test.cpp              # Entity & parsing unit tests
    └── task_store_test.cpp        # Storage, mutation, and persistence round-trip tests
```

---

## 2. Why Each File Exists and Architectural Rules

### Why the Header / Source Split?
- **`include/todo/*.hpp`**: Headers declare interfaces, types, and function signatures. They form the public contract that callers rely on.
- **`src/*.cpp`**: Implementation files compile into object files (`.o`). Separating implementation from declarations drastically reduces rebuild cascades when implementation logic changes.
- **`include/todo/` directory nesting**: Placing headers in a subfolder requires consumers to write `#include "todo/task.hpp"`, eliminating header name collisions with third-party libraries.

### Why the Library / Executable Target Split?
- Core business logic lives in `todo_core` (a CMake `STATIC` library target).
- `main.cpp` is only an executable shell calling `todo::run_cli(argc, argv)`.
- **Benefit:** Test executables (`todo_tests`) can link directly against `todo_core`. If logic lived inside `main.cpp`, tests would either have to recompile all `.cpp` files or couldn't test CLI/storage logic without spawning subprocesses.

### Dependency Hierarchy (Layering Rule)
```
[cli.hpp / cli.cpp]
       │  (depends on)
       ▼
[task_store.hpp / task_store.cpp]
       │  (depends on)
       ▼
[task.hpp / task.cpp]
```
- Dependencies point strictly downward.
- `Task` has no knowledge of how tasks are saved or how commands are parsed.
- `TaskStore` manages task collections and filesystem persistence, but knows nothing about CLI flags.
- `CLI` coordinates user commands and invokes `TaskStore`.

### Testability via Constructor Injection
- `TaskStore` accepts `std::filesystem::path` in its constructor instead of hardcoding `~/.todo/tasks.json`.
- Production code passes the user's home path or environment override (`TODO_TASKS_FILE`).
- Unit tests pass isolated temporary file paths (`/tmp/todo_tests/...`), preventing tests from modifying real user data.

### Predictable Failure Modes (`std::optional`)
- Missing IDs and parsing errors return `std::nullopt` rather than throwing C++ exceptions.
- Exceptions are reserved for catastrophic I/O failures where recovery isn't expected.

---

## 3. Building and Running

### Prerequisites
- C++20 compliant compiler: **GCC 11+**, **Clang 13+**, or **AppleClang 14+**
- **CMake 3.20+**
- **Git** (for CMake `FetchContent` downloads)
- Optional: **Ninja** build system

### Configure and Build
```bash
# Configure the build directory (fetches nlohmann/json, CLI11, and GoogleTest)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Compile all targets
cmake --build build --parallel
```

### Running Unit Tests
```bash
# Run tests using CTest
ctest --test-dir build --output-on-failure

# Or run the test executable directly
./build/todo_tests
```

### Running the Application
```bash
# Show command-line help
./build/todo --help

# Add tasks with optional priority (low, med, high)
./build/todo add "Buy milk" --priority low
./build/todo add "Finish compiler project" --priority high

# List tasks (all, pending, or completed)
./build/todo list
./build/todo list --pending
./build/todo list --done

# Mark a task as complete
./build/todo done 1

# Delete a task
./build/todo remove 2

# Override the storage file path via environment variable
TODO_TASKS_FILE="./my_tasks.json" ./build/todo list
```

### Building with Docker
```bash
# Build the multi-stage Docker container
docker build -t todo:latest .

# Run inside Docker
docker run --rm -it todo:latest list
```

---

## 4. How a C++ Build Happens: The 3 Stages

When you execute `cmake --build build`, the toolchain performs three sequential phases:

```
[Preprocessing] ──> [Compilation] ──> [Linking]
    (#include,           (Syntax AST,      (Symbol resolution,
     macros,              code gen ->       addresses ->
     pragmas)             .o objects)       binary/library)
```

### Stage 1: Preprocessing (`cpp`)
- Expands `#include "todo/task.hpp"` by pasting the exact contents into the translation unit (`.cpp`).
- Evaluates `#pragma once` to prevent duplicate inclusion.
- Expands macro definitions and strips comments.

### Stage 2: Compilation (`cc1plus` / `clang -cc1`)
- Parses preprocessed C++ source into an Abstract Syntax Tree (AST), performs type checking, and emits machine code.
- Each translation unit compiles independently into an object file:
  - `src/task.cpp`       -> `build/.../task.cpp.o`
  - `src/task_store.cpp` -> `build/.../task_store.cpp.o`
  - `src/cli.cpp`        -> `build/.../cli.cpp.o`
  - `src/main.cpp`       -> `build/.../main.cpp.o`
- Object files contain compiled assembly and a symbol table (`T` for defined functions, `U` for undefined external symbols).

### Stage 3: Linking (`ld` / `lld`)
- Resolves all `U` (undefined) symbol references to actual memory addresses in compiled objects and libraries.
- Static Library Step (`ar` / `llvm-ar`):
  - Bundles `task.cpp.o`, `task_store.cpp.o`, and `cli.cpp.o` into an archive: `libtodo_core.a`.
- Executable Linking:
  - Links `main.cpp.o` with `libtodo_core.a` and standard libraries to produce the final executable `todo`.
  - Links test objects with `libtodo_core.a` and `libgtest.a` to produce `todo_tests`.

---

## 5. Top 5 Beginner Multi-File C++ Errors

### 1. Undefined Reference / Unresolved External Symbol (`linker error`)
- **What it is:** The compiler compiled a file because the header declared `void save();`, but the linker could not find any object file containing the actual definition `TaskStore::save() { ... }`.
- **Where it could happen here:** If you add a new method to `include/todo/task_store.hpp` but forget to add `src/task_store.cpp` to `add_library(todo_core ...)` in `CMakeLists.txt`, or forget to write the function body in `src/task_store.cpp`.

### 2. Missing Include Guard / Pragma Once (`compiler error`)
- **What it is:** If two headers include the same header file and `#pragma once` is missing, the preprocessor copies the declaration twice into the same `.cpp` file, triggering `error: redefinition of 'struct Task'`.
- **Where it could happen here:** In `include/todo/task.hpp`. Both `task_store.hpp` and `cli.hpp` include `task.hpp`. If `task.hpp` lacks `#pragma once`, any file including both gets duplicate struct definitions.

### 3. One Definition Rule (ODR) Violation (`linker / runtime error`)
- **What it is:** Defining a non-inline function inside a header file without `inline`. Every `.cpp` file including that header emits an identical function symbol, leading to `multiple definition of 'to_string(Priority)'`.
- **Where it could happen here:** In `include/todo/task.hpp`. If you implement `std::string_view to_string(Priority p) { return ...; }` directly in `task.hpp` instead of `src/task.cpp` (without marking it `inline`), both `task.cpp.o` and `cli.cpp.o` will export the same symbol, failing at link time.

### 4. Circular / Include Cycle (`compiler error`)
- **What it is:** File A includes File B, and File B includes File A. Neither can finish parsing types from the other, causing `unknown type name 'TaskStore'`.
- **Where it could happen here:** If `TaskStore` depended on `CLI`, and `CLI` depended on `TaskStore`. Avoid this by enforcing strictly one-way dependency layers.

### 5. Header-Only vs Compiled Mixups (`build / runtime issues`)
- **What it is:** Treating a compiled library as header-only (forgetting to compile its sources) or re-compiling a header in multiple targets instead of linking.
- **Where it could happen here:** `CLI11` and `nlohmann_json` are header-only libraries, while `GoogleTest` is compiled. CMake handles this automatically via `target_link_libraries`, but forgetting to link `todo_core` in test targets would cause link failures.

---

## 6. Extension Exercises (Increasing Difficulty)

1. **Exercise 1 (Beginner): Due Dates & Sorting**
   - Add an optional `std::optional<std::chrono::year_month_day>` or string ISO date (`YYYY-MM-DD`) to `Task`.
   - Add a `--due <YYYY-MM-DD>` flag to `todo add`.
   - In `todo list`, sort tasks chronologically by due date.

2. **Exercise 2 (Intermediate): Tagging System & Query Filtering**
   - Extend `Task` with `std::vector<std::string> tags`.
   - Update `todo add "Write report" --tag work --tag urgent`.
   - Add `todo list --tag <name>` to filter tasks matching specific tags.
   - Update unit tests to verify tag persistence and multi-tag filtering.

3. **Exercise 3 (Advanced): Storage Interface Abstraction (SQLite / JSON Swap)**
   - Define an abstract interface class `ITaskStore` in `include/todo/task_store_interface.hpp` with pure virtual methods:
     ```cpp
     class ITaskStore {
     public:
         virtual ~ITaskStore() = default;
         virtual uint32_t add_task(std::string title, Priority priority) = 0;
         virtual std::optional<Task> mark_done(uint32_t id) = 0;
         // ...
     };
     ```
   - Make `JsonTaskStore` inherit from `ITaskStore`.
   - Implement `SqliteTaskStore` using SQLite3.
   - Pass `std::unique_ptr<ITaskStore>` into `cli.cpp`, completely decoupling storage implementation from commands.
