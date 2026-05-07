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

// Stubs — implemented in Task 4
std::optional<LogEvent> parse_line(const std::string& line) { return std::nullopt; }
std::vector<LogEvent> parse_chunk(const std::vector<std::string>& lines, int start, int end) { return {}; }
