// src/model.cpp
//
// Implements severity scoring and the two Cluster helper methods.
// All types (LogEvent, Cluster, SEVERITY_ORDER) come from model.hpp.

#include "model.hpp"

// severity_score: look up a level name in SEVERITY_ORDER and return its number
//
// The '&' in "const std::string& level" means pass by reference — no copy is made.
// Like Python passing a string object — fast, no duplication.
// 'const' means we promise not to modify the string inside this function.
int severity_score(const std::string& level) {
    // .find() returns an iterator — think of it as a cursor pointing to the found entry
    // If not found, it returns .end() — a special sentinel meaning "past the end"
    // Like Python: "if key not in dict: return 0"
    auto it = SEVERITY_ORDER.find(level);
    if (it == SEVERITY_ORDER.end()) return 0;

    // 'it' is a pointer to a key/value pair
    // it->first  = the key   (the level string, e.g. "ERROR")
    // it->second = the value (the score, e.g. 50)
    return it->second;
}

// max_severity: find the highest severity score among all levels in this cluster
//
// 'const' after the () means this method doesn't modify 'this' (the Cluster object)
// Like Python's @property — a read-only computed value
int Cluster::max_severity() const {
    int max = 0;

    // Structured binding: "const auto& [level, count]" unpacks each map entry
    // into two named variables — like Python's "for level, count in dict.items():"
    for (const auto& [level, count] : level_counts) {
        int s = severity_score(level);
        if (s > max) max = s;
    }
    return max;
}

// top_level: find the most common level, breaking ties with higher severity
std::string Cluster::top_level() const {
    if (level_counts.empty()) return "UNKNOWN";

    std::string best;
    int best_count = 0;
    int best_sev   = 0;

    for (const auto& [level, count] : level_counts) {
        int sev = severity_score(level);
        // Prefer higher count; break ties by choosing higher severity
        if (count > best_count || (count == best_count && sev > best_sev)) {
            best       = level;
            best_count = count;
            best_sev   = sev;
        }
    }
    return best;
}
