#pragma once

#include <string>
#include <vector>
#include <utility>   // for std::pair
#include "model.hpp"

// format_text_report: human-readable text report
// Mirrors Python's output.py format_text_report()
// Returns a string — caller prints it to stdout
std::string format_text_report(
    const std::vector<std::pair<std::string, Cluster>>& clusters,
    bool show_samples = false);

// clusters_to_json: machine-readable JSON report
// Hand-built string — no external JSON library needed
// Mirrors Python's output.py clusters_to_json()
std::string clusters_to_json(
    const std::vector<std::pair<std::string, Cluster>>& clusters,
    bool include_samples = false);
