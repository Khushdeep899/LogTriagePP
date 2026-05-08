#pragma once

#include <string>
#include <vector>
#include <optional>
#include "model.hpp"

std::string normalize_message(const std::string& msg);
std::optional<LogEvent> parse_line(const std::string& line);
std::vector<LogEvent> parse_chunk(const std::vector<std::string>& lines, int start, int end);
