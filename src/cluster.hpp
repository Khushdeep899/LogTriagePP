#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include "model.hpp"

std::unordered_map<std::string, Cluster> build_clusters(
    const std::vector<LogEvent>& events,
    int max_samples = 5);

std::vector<std::pair<std::string, Cluster>> select_top_clusters(
    const std::unordered_map<std::string, Cluster>& clusters,
    int top_n = 10,
    int min_count = 2,
    int min_severity = 0);
