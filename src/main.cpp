#include "todo/cli.hpp"

// main.cpp is intentionally minimal (< 20 lines).
// All domain and CLI parsing logic resides in the static library target (todo_core),
// enabling seamless testing and decoupling.
int main(int argc, char* argv[]) {
    return todo::run_cli(argc, argv);
}
