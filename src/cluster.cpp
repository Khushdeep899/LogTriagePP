#include "cluster.hpp"
#include "model.hpp"
#include <algorithm>

static void update_cluster(Cluster& cluster, const LogEvent& event, int max_samples) {
    cluster.count++;

    if (event.timestamp.has_value()) {
        if (!cluster.first_ts.has_value()) cluster.first_ts = event.timestamp;
        cluster.last_ts = event.timestamp;
    }

    cluster.level_counts[event.level]++;

    if (static_cast<int>(cluster.samples.size()) < max_samples) {
        cluster.samples.push_back(event.message);
    }
}

std::unordered_map<std::string, Cluster> build_clusters(
    const std::vector<LogEvent>& events, int max_samples)
{
    std::unordered_map<std::string, Cluster> clusters;
    for (const auto& event : events) {
        std::string key = event.component + "|" + event.normalized;
        update_cluster(clusters[key], event, max_samples);
    }
    return clusters;
}

std::vector<std::pair<std::string, Cluster>> select_top_clusters(
    const std::unordered_map<std::string, Cluster>& clusters,
    int top_n, int min_count, int min_severity)
{
    std::vector<std::pair<std::string, Cluster>> result;

    for (const auto& [key, cluster] : clusters) {
        if (cluster.count < min_count)             continue;
        if (cluster.max_severity() < min_severity) continue;
        result.push_back({key, cluster});
    }

    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) {
            int sev_a = a.second.max_severity();
            int sev_b = b.second.max_severity();
            if (sev_a != sev_b) return sev_a > sev_b;
            return a.second.count > b.second.count;
        }
    );

    if (static_cast<int>(result.size()) > top_n) {
        result.resize(top_n);
    }

    return result;
}
