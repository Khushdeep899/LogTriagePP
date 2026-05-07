#pragma once

// #pragma once tells the compiler to include this file only once
// even if multiple files #include it — like Python's import deduplication

#include <string>
#include <vector>
#include <map>
#include <optional>

// A struct is like a Python @dataclass — a named bundle of fields
// Unlike Python, we must declare the type of each field upfront

// std::optional<T> means "this value might not exist"
// Like writing "str | None" in Python type hints
// Access with .value() or check with .has_value()
struct LogEvent {
    std::optional<std::string> timestamp; // might be absent on some log lines
    std::string level;                    // "ERROR", "WARN", "INFO", etc.
    std::string component;               // "auth", "db", "api", etc.
    std::string message;                 // raw message text
    std::string normalized;              // message with UUIDs/IPs/numbers replaced
};

struct Cluster {
    int count = 0;                              // default-initialized to 0
    std::optional<std::string> first_ts;        // earliest timestamp seen
    std::optional<std::string> last_ts;         // latest timestamp seen
    std::map<std::string, int> level_counts;    // like Python's Counter
    std::vector<std::string> samples;           // example messages, like Python's list

    // 'const' at the end means this method does NOT modify the struct
    // Like Python's @property — read-only computed value
    int max_severity() const;
    std::string top_level() const;
};

// Same as Python's SEVERITY_ORDER = {"TRACE": 10, "DEBUG": 20, ...}
// 'inline' prevents "multiple definition" errors when this header is included
// from many different .cpp files
inline const std::map<std::string, int> SEVERITY_ORDER = {
    {"TRACE", 10}, {"DEBUG", 20}, {"INFO", 30},
    {"WARN", 40},  {"WARNING", 40}, {"ERROR", 50},
    {"CRITICAL", 60}, {"FATAL", 70}
};

// Returns numeric severity for a level string, defaults to 0 if unknown
int severity_score(const std::string& level);
