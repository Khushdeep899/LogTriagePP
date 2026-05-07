#pragma once

// I/O helpers — mirrors Python's io.py
// Reads lines from files or stdin into a vector of strings

#include <string>
#include <vector>

// Read all lines from a list of file paths
// Opens each file, reads line by line, returns all lines in order
// Prints a warning to stderr if a file can't be opened (same as Python)
std::vector<std::string> read_lines_from_files(const std::vector<std::string>& paths);

// Read all lines from standard input (stdin)
// Used when no --file flags are given — same as Python's stdin fallback
std::vector<std::string> read_lines_from_stdin();
