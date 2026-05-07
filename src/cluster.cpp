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

// select_top_clusters: filter clusters and return the most important ones
//
// Filters by min_count and min_severity, then sorts by severity (desc) then
// count (desc), then returns up to top_n results.
std::vector<std::pair<std::string, Cluster>> select_top_clusters(
    const std::unordered_map<std::string, Cluster>& clusters,
    int top_n, int min_count, int min_severity)
{
    // Collect clusters that pass both filters into a sortable vector
    // std::pair<K,V> is a two-element struct — like Python's (key, value) tuple
    std::vector<std::pair<std::string, Cluster>> result;

    for (const auto& [key, cluster] : clusters) {
        if (cluster.count < min_count)             continue;  // too few occurrences
        if (cluster.max_severity() < min_severity) continue;  // not severe enough
        result.push_back({key, cluster});
    }

    // std::sort with a lambda comparator
    // Lambda syntax: [captures](params) { body }
    // [] means no variables from the outer scope are captured
    // Like Python's sorted(result, key=lambda x: (-x[1].max_severity(), -x[1].count))
    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) {
            int sev_a = a.second.max_severity();
            int sev_b = b.second.max_severity();
            if (sev_a != sev_b) return sev_a > sev_b;   // higher severity first
            return a.second.count > b.second.count;       // then higher count first
        }
    );

    // Trim to top_n — like Python's result[:top_n]
    if (static_cast<int>(result.size()) > top_n) {
        result.resize(top_n);
    }

    return result;
}
