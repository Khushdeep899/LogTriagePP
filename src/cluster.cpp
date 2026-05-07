// src/cluster.cpp
//
// Groups parsed log events into clusters and selects the most important ones.
// This is the C++ equivalent of Python's cluster.py.

#include "cluster.hpp"
#include "model.hpp"
#include <algorithm>  // std::sort

// update_cluster: merge one event into an existing Cluster
//
// Takes cluster by reference (&) so changes persist — like passing a mutable
// Python object: the function modifies it in-place.
static void update_cluster(Cluster& cluster, const LogEvent& event, int max_samples) {
    cluster.count++;

    // Update the timestamp window if this event has one
    if (event.timestamp.has_value()) {
        // Set first_ts only on the very first event
        if (!cluster.first_ts.has_value()) cluster.first_ts = event.timestamp;
        cluster.last_ts = event.timestamp;
    }

    // operator[] on a map creates the entry with value 0 if the key is missing
    // Same as Python's collections.defaultdict(int) — no KeyError
    cluster.level_counts[event.level]++;

    // Keep up to max_samples raw example messages
    if (static_cast<int>(cluster.samples.size()) < max_samples) {
        cluster.samples.push_back(event.message);
    }
}

// build_clusters: group events by "component|normalized_message" key
//
// std::unordered_map is like Python's dict — O(1) average lookup by key.
// The key combines component and normalized message with '|' as separator,
// same as Python's (component, normalized) tuple key.
std::unordered_map<std::string, Cluster> build_clusters(
    const std::vector<LogEvent>& events, int max_samples)
{
    std::unordered_map<std::string, Cluster> clusters;

    for (const auto& event : events) {
        // Build the grouping key — same logic as Python's build_clusters()
        std::string key = event.component + "|" + event.normalized;

        // clusters[key] creates an empty Cluster if key doesn't exist yet
        // Then update_cluster fills it in — like Python's setdefault + update
        update_cluster(clusters[key], event, max_samples);
    }

    return clusters;
}

// Stub — implemented in Task 6
std::vector<std::pair<std::string, Cluster>> select_top_clusters(
    const std::unordered_map<std::string, Cluster>& clusters,
    int top_n, int min_count, int min_severity) { return {}; }
