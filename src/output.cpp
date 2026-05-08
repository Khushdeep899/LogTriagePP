#include "output.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <algorithm>

static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else                out += c;
    }
    return out;
}

std::string format_text_report(
    const std::vector<std::pair<std::string, Cluster>>& clusters,
    bool show_samples)
{
    std::ostringstream out;

    out << "\n=== LogTriage++ Report ===\n";
    out << "Found " << clusters.size() << " cluster(s)\n\n";

    int i = 1;
    for (const auto& [key, cluster] : clusters) {
        auto sep = key.find('|');
        std::string component = (sep != std::string::npos) ? key.substr(0, sep) : key;
        std::string signature = (sep != std::string::npos) ? key.substr(sep + 1) : key;

        out << "--- Cluster " << i++ << " ---\n";
        out << "  Count:     " << cluster.count       << "\n";
        out << "  Top Level: " << cluster.top_level() << "\n";
        out << "  Component: " << component           << "\n";
        out << "  Signature: " << signature           << "\n";

        if (cluster.first_ts.has_value())
            out << "  First:     " << cluster.first_ts.value() << "\n";
        if (cluster.last_ts.has_value())
            out << "  Last:      " << cluster.last_ts.value()  << "\n";

        out << "  Levels:    ";
        std::vector<std::pair<std::string, int>> sorted_levels(
            cluster.level_counts.begin(), cluster.level_counts.end()
        );
        std::sort(sorted_levels.begin(), sorted_levels.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        bool first = true;
        for (const auto& [level, count] : sorted_levels) {
            if (!first) out << ", ";
            out << level << "=" << count;
            first = false;
        }
        out << "\n";

        if (show_samples && !cluster.samples.empty()) {
            out << "  Samples:\n";
            int shown = 0;
            for (const auto& sample : cluster.samples) {
                if (shown++ >= 3) break;
                out << "    - " << sample << "\n";
            }
        }
        out << "\n";
    }

    return out.str();
}

std::string clusters_to_json(
    const std::vector<std::pair<std::string, Cluster>>& clusters,
    bool include_samples)
{
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ts;
    ts << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");

    std::ostringstream out;
    out << "{\n";
    out << "  \"generated_at\": \"" << ts.str() << "\",\n";
    out << "  \"clusters\": [\n";

    bool first_cluster = true;
    for (const auto& [key, cluster] : clusters) {
        if (!first_cluster) out << ",\n";
        first_cluster = false;

        auto sep = key.find('|');
        std::string component = (sep != std::string::npos) ? key.substr(0, sep) : key;
        std::string signature = (sep != std::string::npos) ? key.substr(sep + 1) : key;

        out << "    {\n";
        out << "      \"component\": \"" << json_escape(component) << "\",\n";
        out << "      \"signature\": \"" << json_escape(signature) << "\",\n";
        out << "      \"count\": "       << cluster.count          << ",\n";
        out << "      \"top_level\": \"" << cluster.top_level()    << "\",\n";
        out << "      \"levels\": {";

        bool first_level = true;
        for (const auto& [level, count] : cluster.level_counts) {
            if (!first_level) out << ", ";
            out << "\"" << level << "\": " << count;
            first_level = false;
        }
        out << "}";

        if (include_samples && !cluster.samples.empty()) {
            out << ",\n      \"samples\": [";
            bool first_sample = true;
            for (const auto& sample : cluster.samples) {
                if (!first_sample) out << ", ";
                out << "\"" << json_escape(sample) << "\"";
                first_sample = false;
            }
            out << "]";
        }

        out << "\n    }";
    }

    out << "\n  ]\n}\n";
    return out.str();
}
