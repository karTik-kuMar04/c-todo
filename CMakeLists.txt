cmake_minimum_required(VERSION 3.20)
project(todo_manager VERSION 1.0.0 LANGUAGES CXX)

# Enforce C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for clangd / language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Strict warning flags across GCC and Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Dependency management via FetchContent
include(FetchContent)

# 1. nlohmann/json (modern JSON serialization for C++)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

# 2. CLI11 (modern command-line parser)
FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.4.2
)

# 3. GoogleTest (unit testing framework)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# Make dependencies available
FetchContent_MakeAvailable(nlohmann_json cli11 googletest)

# Enable CTest integration
include(CTest)
enable_testing()

# ---------------------------------------------------------------------------
# Target 1: Static Library (Core Logic)
# Separating logic into a library allows both the executable and test runner
# to link against the exact same compiled object code.
# ---------------------------------------------------------------------------
add_library(todo_core STATIC
    src/task.cpp
    src/task_store.cpp
    src/cli.cpp
)

target_include_directories(todo_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(todo_core PUBLIC
    nlohmann_json::nlohmann_json
    CLI11::CLI11
)

# ---------------------------------------------------------------------------
# Target 2: Executable (CLI entrypoint)
# Intentionally minimal: links against todo_core and calls run_cli().
# ---------------------------------------------------------------------------
add_executable(todo src/main.cpp)
target_link_libraries(todo PRIVATE todo_core)

# ---------------------------------------------------------------------------
# Target 3: Unit Tests
# ---------------------------------------------------------------------------
if(BUILD_TESTING)
    add_executable(todo_tests
        tests/task_test.cpp
        tests/task_store_test.cpp
    )
    target_link_libraries(todo_tests PRIVATE
        todo_core
        GTest::gtest_main
    )
    include(GoogleTest)
    gtest_discover_tests(todo_tests)
endif()
