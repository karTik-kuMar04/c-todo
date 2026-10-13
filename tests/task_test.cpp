#include <gtest/gtest.h>
#include "todo/task.hpp"

namespace todo {
namespace {

TEST(TaskTest, PriorityToStringConversion) {
    EXPECT_EQ(to_string(Priority::Low), "low");
    EXPECT_EQ(to_string(Priority::Med), "med");
    EXPECT_EQ(to_string(Priority::High), "high");
}

TEST(TaskTest, ParsePriorityValidStrings) {
    EXPECT_EQ(parse_priority("low"), Priority::Low);
    EXPECT_EQ(parse_priority("LOW"), Priority::Low);
    EXPECT_EQ(parse_priority("Med"), Priority::Med);
    EXPECT_EQ(parse_priority("high"), Priority::High);
    EXPECT_EQ(parse_priority("HIGH"), Priority::High);
}

TEST(TaskTest, ParsePriorityInvalidStrings) {
    EXPECT_FALSE(parse_priority("urgent").has_value());
    EXPECT_FALSE(parse_priority("").has_value());
    EXPECT_FALSE(parse_priority("123").has_value());
}

TEST(TaskTest, JsonSerializationRoundTrip) {
    Task original{
        .id = 42,
        .title = "Learn CMake FetchContent",
        .priority = Priority::High,
        .done = true
    };

    nlohmann::json j = original;
    Task deserialized = j.get<Task>();

    EXPECT_EQ(original, deserialized);
    EXPECT_EQ(deserialized.id, 42);
    EXPECT_EQ(deserialized.title, "Learn CMake FetchContent");
    EXPECT_EQ(deserialized.priority, Priority::High);
    EXPECT_TRUE(deserialized.done);
}

} // namespace
} // namespace todo
