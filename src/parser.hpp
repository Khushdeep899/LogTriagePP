#pragma once

// .hpp files declare WHAT exists — like a table of contents
// .cpp files define HOW it works — the actual code
// This split lets other files use these functions without re-compiling them each time

#include <string>
#include <vector>
#include <optional>  // for std::optional — the "might be None" type
#include "model.hpp"

// normalize_message: replace variable parts of a message with fixed placeholders
// Input:  "user connected from 192.168.1.1 took 123ms"
// Output: "user connected from <IP> took <NUM>ms"
// This makes two messages that differ only in IDs/IPs/numbers cluster together
std::string normalize_message(const std::string& msg);

// parse_line: extract structured fields from a single raw log line
// Returns std::nullopt (like Python's None) if the line is blank or unparseable
// The '&' means we pass the string by reference — no copy, more efficient
std::optional<LogEvent> parse_line(const std::string& line);

// parse_chunk: parse a slice of lines [start, end)
// Called by each std::thread with its own non-overlapping slice
// Returns all successfully parsed events from that slice
std::vector<LogEvent> parse_chunk(const std::vector<std::string>& lines,
                                   int start, int end);
