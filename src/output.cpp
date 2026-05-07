// src/output.cpp
//
// Formats clusters as human-readable text or machine-readable JSON.
// This is the C++ equivalent of Python's output.py.

#include "output.hpp"
#include <sstream>    // std::ostringstream — like Python's io.StringIO
#include <iomanip>    // std::put_time — for timestamp formatting
#include <chrono>     // std::chrono::system_clock — current wall-clock time
#include <ctime>      // std::gmtime — convert time_t to UTC struct
#include <algorithm>  // std::sort

// Helper: escape special characters so they're safe inside a JSON string value
// Without this, a message like 'say "hello"' would break the JSON structure
static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());  // pre-allocate — like Python's list with capacity hint
    for (char c : s) {
        if      (c == '"')  out += "\\\"";   // " → \"
        else if (c == '\\') out += "\\\\";   // \ → \\
        else if (c == '\n') out += "\\n";    // newline → \n
        else if (c == '\r') out += "\\r";    // carriage return → \r
        else                out += c;
    }
    return out;
}

// format_text_report: build a human-readable text report string
//
// std::ostringstream builds a string piece by piece — like Python's io.StringIO
// We return the whole string at the end; the caller prints it to stdout.
std::string format_text_report(
    const std::vector<std::pair<std::string, Cluster>>& clusters,
    bool show_samples)
{
    std::ostringstream out;

    out << "\n=== LogTriage++ Report ===\n";
    out << "Found " << clusters.size() << " cluster(s)\n\n";

    int i = 1;
    for (const auto& [key, cluster] : clusters) {
        // Key format is "component|signature" — split on the separator
        auto sep       = key.find('|');
        std::string component = (sep != std::string::npos) ? key.substr(0, sep) : key;
        std::string signature = (sep != std::string::npos) ? key.substr(sep + 1) : key;

        out << "--- Cluster " << i++ << " ---\n";
        out << "  Count:     " << cluster.count       << "\n";
        out << "  Top Level: " << cluster.top_level() << "\n";
        out << "  Component: " << component           << "\n";
        out << "  Signature: " << signature           << "\n";

        // .has_value() checks if the optional contains a value — like "is not None"
        // .value() unwraps the optional to get the actual string
        if (cluster.first_ts.has_value())
            out << "  First:     " << cluster.first_ts.value() << "\n";
        if (cluster.last_ts.has_value())
            out << "  Last:      " << cluster.last_ts.value()  << "\n";

        // Print level distribution sorted by count descending
        // Must copy into a vector first since std::map doesn't support sort()
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
                if (shown++ >= 3) break;  // cap at 3 examples
                out << "    - " << sample << "\n";
            }
        }
        out << "\n";
    }

    return out.str();  // .str() extracts the built string — like StringIO.getvalue()
}

// clusters_to_json: build a JSON report string
//
// Hand-built string — no external JSON library needed.
// Mirrors Python's clusters_to_json() output structure.
std::string clusters_to_json(
    const std::vector<std::pair<std::string, Cluster>>& clusters,
    bool include_samples)
{
    // Get current UTC time as an ISO 8601 string — like Python's datetime.utcnow()
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ts;
    // std::put_time formats time — like Python's strftime("%Y-%m-%dT%H:%M:%SZ")
    // std::gmtime converts to UTC (not local time)
    ts << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");

    std::ostringstream out;
    out << "{\n";
    out << "  \"generated_at\": \"" << ts.str() << "\",\n";
    out << "  \"clusters\": [\n";

    bool first_cluster = true;
    for (const auto& [key, cluster] : clusters) {
        if (!first_cluster) out << ",\n";
        first_cluster = false;

        auto sep       = key.find('|');
        std::string component = (sep != std::string::npos) ? key.substr(0, sep) : key;
        std::string signature = (sep != std::string::npos) ? key.substr(sep + 1) : key;

        out << "    {\n";
        out << "      \"component\": \"" << json_escape(component)     << "\",\n";
        out << "      \"signature\": \"" << json_escape(signature)     << "\",\n";
        out << "      \"count\": "       << cluster.count              << ",\n";
        out << "      \"top_level\": \"" << cluster.top_level()        << "\",\n";
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
