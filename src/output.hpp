#pragma once

#include <string>
#include <vector>
#include <utility>
#include "model.hpp"

std::string format_text_report(
    const std::vector<std::pair<std::string, Cluster>>& clusters,
    bool show_samples = false);

std::string clusters_to_json(
    const std::vector<std::pair<std::string, Cluster>>& clusters,
    bool include_samples = false);
