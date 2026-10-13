#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <nlohmann/json.hpp>

namespace todo {

enum class Priority {
    Low,
    Med,
    High
};

[[nodiscard]] std::string_view to_string(Priority p) noexcept;
[[nodiscard]] std::optional<Priority> parse_priority(std::string_view str) noexcept;

struct Task {
    uint32_t id{0};
    std::string title{};
    Priority priority{Priority::Med};
    bool done{false};

    // Equality comparison operator (C++20 default comparison)
    bool operator==(const Task& other) const = default;
};

// nlohmann::json serialization ADL hooks
void to_json(nlohmann::json& j, const Task& t);
void from_json(const nlohmann::json& j, Task& t);

} // namespace todo
