#pragma once

#include <string>
#include <vector>
#include <unordered_map>  // like Python's dict — O(1) average lookup
#include <utility>        // for std::pair
#include "model.hpp"

// build_clusters: group events by (component + "|" + normalized_message)
// Returns a map where key = "component|signature", value = Cluster
// Like Python's dict-based grouping with a defaultdict(Cluster)
//
// max_samples controls how many raw example messages to store per cluster
std::unordered_map<std::string, Cluster> build_clusters(
    const std::vector<LogEvent>& events,
    int max_samples = 5);

// select_top_clusters: filter and rank clusters
// Removes clusters with fewer than min_count events or below min_severity
// Sorts by severity descending, then count descending
// Returns up to top_n results as (key, Cluster) pairs
//
// std::vector<std::pair<K,V>> is like Python's list of (key, value) tuples
std::vector<std::pair<std::string, Cluster>> select_top_clusters(
    const std::unordered_map<std::string, Cluster>& clusters,
    int top_n = 10,
    int min_count = 2,
    int min_severity = 0);
