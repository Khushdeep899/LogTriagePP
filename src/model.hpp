#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

struct LogEvent {
    std::optional<std::string> timestamp;
    std::string level;
    std::string component;
    std::string message;
    std::string normalized;
};

struct Cluster {
    int count = 0;
    std::optional<std::string> first_ts;
    std::optional<std::string> last_ts;
    std::map<std::string, int> level_counts;
    std::vector<std::string> samples;

    int max_severity() const;
    std::string top_level() const;
};

inline const std::map<std::string, int> SEVERITY_ORDER = {
    {"TRACE", 10}, {"DEBUG", 20}, {"INFO", 30},
    {"WARN", 40},  {"WARNING", 40}, {"ERROR", 50},
    {"CRITICAL", 60}, {"FATAL", 70}
};

int severity_score(const std::string& level);
