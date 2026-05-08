#include "parser.hpp"
#include "constants.hpp"
#include <regex>

std::string normalize_message(const std::string& msg) {
    std::string result = msg;
    for (const auto& [pattern, replacement] : NORMALIZE_RULES) {
        result = std::regex_replace(result, std::regex(pattern), replacement);
    }
    return result;
}

std::optional<LogEvent> parse_line(const std::string& line) {
    if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
        return std::nullopt;
    }

    static const std::regex log_re(LOG_LINE_PATTERN);
    std::smatch m;
    LogEvent event;

    if (std::regex_search(line, m, log_re)) {
        if (m[1].matched) event.timestamp = m[1].str();
        event.level     = m[2].matched ? m[2].str() : "UNKNOWN";
        event.component = m[3].matched ? m[3].str() : "UNKNOWN";
        event.message   = m[4].matched ? m[4].str() : line;
    } else {
        event.level     = "UNKNOWN";
        event.component = "UNKNOWN";
        event.message   = line;
    }

    event.normalized = normalize_message(event.message);
    return event;
}

std::vector<LogEvent> parse_chunk(const std::vector<std::string>& lines, int start, int end) {
    std::vector<LogEvent> events;
    for (int i = start; i < end && i < static_cast<int>(lines.size()); ++i) {
        auto opt = parse_line(lines[i]);
        if (opt.has_value()) {
            events.push_back(opt.value());
        }
    }
    return events;
}
