#include "todo/task.hpp"
#include <algorithm>
#include <cctype>

namespace todo {

std::string_view to_string(Priority p) noexcept {
    switch (p) {
        case Priority::Low:  return "low";
        case Priority::Med:  return "med";
        case Priority::High: return "high";
    }
    return "med";
}

std::optional<Priority> parse_priority(std::string_view str) noexcept {
    std::string lower;
    lower.reserve(str.size());
    for (char ch : str) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    if (lower == "low")  return Priority::Low;
    if (lower == "med")  return Priority::Med;
    if (lower == "high") return Priority::High;

    return std::nullopt;
}

void to_json(nlohmann::json& j, const Task& t) {
    j = nlohmann::json{
        {"id", t.id},
        {"title", t.title},
        {"priority", std::string(to_string(t.priority))},
        {"done", t.done}
    };
}

void from_json(const nlohmann::json& j, Task& t) {
    j.at("id").get_to(t.id);
    j.at("title").get_to(t.title);
    
    std::string prio_str;
    j.at("priority").get_to(prio_str);
    auto parsed = parse_priority(prio_str);
    t.priority = parsed.value_or(Priority::Med);

    j.at("done").get_to(t.done);
}

} // namespace todo
