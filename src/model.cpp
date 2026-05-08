#include "model.hpp"

int severity_score(const std::string& level) {
    auto it = SEVERITY_ORDER.find(level);
    if (it == SEVERITY_ORDER.end()) return 0;
    return it->second;
}

int Cluster::max_severity() const {
    int max = 0;
    for (const auto& [level, count] : level_counts) {
        int s = severity_score(level);
        if (s > max) max = s;
    }
    return max;
}

std::string Cluster::top_level() const {
    if (level_counts.empty()) return "UNKNOWN";

    std::string best;
    int best_count = 0;
    int best_sev   = 0;

    for (const auto& [level, count] : level_counts) {
        int sev = severity_score(level);
        if (count > best_count || (count == best_count && sev > best_sev)) {
            best       = level;
            best_count = count;
            best_sev   = sev;
        }
    }
    return best;
}
