// src/parser.cpp
//
// Implements log line parsing and message normalization.
// This is the C++ equivalent of Python's parser.py.

#include "parser.hpp"
#include "constants.hpp"
#include <regex>  // std::regex, std::regex_replace, std::smatch

// normalize_message: apply each substitution rule in order
//
// std::regex_replace is like Python's re.sub(pattern, replacement, text)
// Each call scans the whole string and replaces all non-overlapping matches.
std::string normalize_message(const std::string& msg) {
    std::string result = msg;

    // Range-based for loop — like "for pattern, replacement in NORMALIZE_RULES:"
    // 'const auto& [pattern, replacement]' is a structured binding — unpacks the pair
    // 'auto&' means "deduce the type automatically, take by reference"
    for (const auto& [pattern, replacement] : NORMALIZE_RULES) {
        // std::regex compiles a pattern string into a regex object
        // std::regex_replace returns a new string with all matches replaced
        result = std::regex_replace(result, std::regex(pattern), replacement);
    }

    return result;
}

// parse_line: extract structured fields from a single raw log line
//
// Returns std::optional<LogEvent> — either a LogEvent or nothing (nullopt)
// Like Python returning None for blank/unparseable lines
std::optional<LogEvent> parse_line(const std::string& line) {
    // Skip blank or whitespace-only lines
    // find_first_not_of returns npos if every character is in the set
    if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
        return std::nullopt;  // like Python's "return None"
    }

    // 'static' means this variable is created once and reused on every call
    // Compiling a regex is expensive — this is like Python's re.compile() at module level
    static const std::regex log_re(LOG_LINE_PATTERN);

    // std::smatch holds capture group results — like Python's match.groups()
    // 's' in smatch stands for 'string' match
    std::smatch m;
    LogEvent event;

    if (std::regex_search(line, m, log_re)) {
        // m[0] = whole match, m[1] = group 1 (timestamp), m[2] = level, etc.
        // .matched is true if the group participated in the match
        // .str() converts the match object to a plain std::string
        if (m[1].matched) event.timestamp = m[1].str();
        event.level     = m[2].matched ? m[2].str() : "UNKNOWN";
        event.component = m[3].matched ? m[3].str() : "UNKNOWN";
        event.message   = m[4].matched ? m[4].str() : line;
    } else {
        // Line didn't match the pattern — treat whole line as message
        // Same fallback as Python: level=UNKNOWN, component=UNKNOWN
        event.level     = "UNKNOWN";
        event.component = "UNKNOWN";
        event.message   = line;
    }

    event.normalized = normalize_message(event.message);
    return event;
}

// parse_chunk: parse a slice of lines [start, end)
//
// Called by each std::thread with its own non-overlapping slice.
// Returns a vector of all successfully parsed events.
// 'end' is exclusive — lines[start] through lines[end-1] are processed.
std::vector<LogEvent> parse_chunk(const std::vector<std::string>& lines,
                                   int start, int end) {
    std::vector<LogEvent> events;

    // static_cast<int>(...) converts size_t (unsigned) to int — avoids a
    // signed/unsigned comparison compiler warning on line.size()
    for (int i = start; i < end && i < static_cast<int>(lines.size()); ++i) {
        auto opt = parse_line(lines[i]);
        // Check if the optional has a value before using it
        if (opt.has_value()) {
            // .push_back() appends to the vector — like Python's list.append()
            events.push_back(opt.value());
        }
    }

    return events;
}
