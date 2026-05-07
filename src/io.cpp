// src/io.cpp
//
// Reads log lines from files or stdin.
// This is the C++ equivalent of Python's io.py.

#include "io.hpp"
#include <fstream>    // std::ifstream — like Python's open()
#include <iostream>   // std::cin, std::cerr

std::vector<std::string> read_lines_from_files(const std::vector<std::string>& paths) {
    std::vector<std::string> lines;

    for (const auto& path : paths) {
        // std::ifstream opens a file for reading
        // Like Python's "f = open(path, 'r', encoding='utf-8')"
        std::ifstream file(path);

        if (!file.is_open()) {
            // std::cerr is the error output stream — like Python's sys.stderr.write()
            // Using "\n" rather than std::endl avoids flushing on every line
            std::cerr << "Warning: could not open file: " << path << "\n";
            continue;  // skip this file, keep processing others
        }

        std::string line;
        // std::getline reads one line at a time and strips the newline character
        // The while loop runs until end-of-file — like Python's "for line in f:"
        while (std::getline(file, line)) {
            // .push_back() appends to the vector — like Python's list.append()
            lines.push_back(line);
        }

        // 'file' goes out of scope here and closes automatically — no file.close() needed
        // This is RAII: Resource Acquisition Is Initialization
        // The file's destructor runs when the variable goes out of scope
        // Same idea as Python's "with open(...) as f:" — automatic cleanup
    }

    return lines;
}

std::vector<std::string> read_lines_from_stdin() {
    std::vector<std::string> lines;
    std::string line;
    // std::cin is standard input — like reading from Python's sys.stdin
    while (std::getline(std::cin, line)) {
        lines.push_back(line);
    }
    return lines;
}
