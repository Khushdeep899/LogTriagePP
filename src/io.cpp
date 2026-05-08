#include "io.hpp"
#include <fstream>
#include <iostream>

std::vector<std::string> read_lines_from_files(const std::vector<std::string>& paths) {
    std::vector<std::string> lines;
    for (const auto& path : paths) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Warning: could not open file: " << path << "\n";
            continue;
        }
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
    }
    return lines;
}

std::vector<std::string> read_lines_from_stdin() {
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(std::cin, line)) {
        lines.push_back(line);
    }
    return lines;
}
