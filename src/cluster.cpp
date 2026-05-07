#include "cluster.hpp"

std::unordered_map<std::string, Cluster> build_clusters(const std::vector<LogEvent>& events, int max_samples) { return {}; }
std::vector<std::pair<std::string, Cluster>> select_top_clusters(const std::unordered_map<std::string, Cluster>& clusters, int top_n, int min_count, int min_severity) { return {}; }
