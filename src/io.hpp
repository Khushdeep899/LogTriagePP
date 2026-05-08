#pragma once

#include <string>
#include <vector>

std::vector<std::string> read_lines_from_files(const std::vector<std::string>& paths);
std::vector<std::string> read_lines_from_stdin();
