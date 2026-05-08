#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <stdexcept>
#include <algorithm>

#include "model.hpp"
#include "io.hpp"
#include "parser.hpp"
#include "cluster.hpp"
#include "output.hpp"

struct Config {
    std::vector<std::string> files;
    int         threads      = 4;
    int         chunk_size   = 500;
    int         top_n        = 10;
    int         min_count    = 2;
    std::string issue_level  = "any";
    std::string output_fmt   = "text";
    bool        show_samples = false;
    int         max_samples  = 3;
    bool        verbose      = false;
    bool        run_tests    = false;
};

static int issue_min_severity(const std::string& level) {
    if (level == "warn")  return 40;
    if (level == "error") return 50;
    return 0;
}

static void print_usage(const char* prog) {
    std::cerr
        << "Usage: " << prog << " [options]\n"
        << "  --file <path>           Log file to parse (repeatable)\n"
        << "  --threads <n>           Worker threads (default: 4)\n"
        << "  --chunk-size <n>        Lines per thread chunk (default: 500)\n"
        << "  --top <n>               Max clusters to show (default: 10)\n"
        << "  --min-count <n>         Min occurrences to include (default: 2)\n"
        << "  --issue-level <level>   any|warn|error (default: any)\n"
        << "  --output <fmt>          text|json (default: text)\n"
        << "  --samples               Show sample log lines per cluster\n"
        << "  --max-samples <n>       Max samples per cluster (default: 3)\n"
        << "  --verbose               Print progress to stderr\n"
        << "  --run-tests             Show command to run unit tests\n";
}

static Config parse_args(int argc, char* argv[]) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next_arg = [&]() -> std::string {
            if (i + 1 >= argc)
                throw std::runtime_error("Missing value for " + arg);
            return std::string(argv[++i]);
        };

        if      (arg == "--file")         cfg.files.push_back(next_arg());
        else if (arg == "--threads")      cfg.threads      = std::stoi(next_arg());
        else if (arg == "--chunk-size")   cfg.chunk_size   = std::stoi(next_arg());
        else if (arg == "--top")          cfg.top_n        = std::stoi(next_arg());
        else if (arg == "--min-count")    cfg.min_count    = std::stoi(next_arg());
        else if (arg == "--issue-level")  cfg.issue_level  = next_arg();
        else if (arg == "--output")       cfg.output_fmt   = next_arg();
        else if (arg == "--samples")      cfg.show_samples = true;
        else if (arg == "--max-samples")  cfg.max_samples  = std::stoi(next_arg());
        else if (arg == "--verbose")      cfg.verbose      = true;
        else if (arg == "--run-tests")    cfg.run_tests    = true;
        else {
            std::cerr << "Unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            std::exit(2);
        }
    }
    return cfg;
}

int main(int argc, char* argv[]) {
    Config cfg;
    try {
        cfg = parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        print_usage(argv[0]);
        return 2;
    }

    if (cfg.issue_level != "any" && cfg.issue_level != "warn" && cfg.issue_level != "error") {
        std::cerr << "Invalid --issue-level: " << cfg.issue_level << "\n";
        return 2;
    }
    if (cfg.output_fmt != "text" && cfg.output_fmt != "json") {
        std::cerr << "Invalid --output: " << cfg.output_fmt << "\n";
        return 2;
    }

    if (cfg.run_tests) {
        std::cout << "To run unit tests:\n"
                  << "  cd build && ./logtriage_tests\n"
                  << "  (or: cd build && ctest -V)\n";
        return 0;
    }

    std::vector<std::string> lines;
    if (!cfg.files.empty()) {
        lines = read_lines_from_files(cfg.files);
    } else {
        lines = read_lines_from_stdin();
    }

    if (cfg.verbose)
        std::cerr << "[verbose] Read " << lines.size() << " lines\n";

    if (lines.empty()) {
        std::cerr << "No input lines to process.\n";
        return 0;
    }

    int total     = static_cast<int>(lines.size());
    int n_chunks  = std::max(1, (total + cfg.chunk_size - 1) / cfg.chunk_size);
    int n_threads = std::min(cfg.threads, n_chunks);
    int chunk     = (total + n_threads - 1) / n_threads;

    if (cfg.verbose)
        std::cerr << "[verbose] Spawning " << n_threads << " thread(s)\n";

    std::vector<std::vector<LogEvent>> thread_results(n_threads);
    std::vector<std::thread> threads;

    for (int t = 0; t < n_threads; ++t) {
        int start = t * chunk;
        int end   = std::min(start + chunk, total);
        threads.emplace_back(
            [&lines, &thread_results, t, start, end]() {
                thread_results[t] = parse_chunk(lines, start, end);
            }
        );
    }

    for (auto& th : threads) {
        th.join();
    }

    std::vector<LogEvent> all_events;
    for (const auto& part : thread_results) {
        all_events.insert(all_events.end(), part.begin(), part.end());
    }

    if (cfg.verbose)
        std::cerr << "[verbose] Parsed " << all_events.size() << " events\n";

    auto clusters = build_clusters(all_events, cfg.max_samples);
    int  min_sev  = issue_min_severity(cfg.issue_level);
    auto top      = select_top_clusters(clusters, cfg.top_n, cfg.min_count, min_sev);

    if (cfg.verbose)
        std::cerr << "[verbose] " << top.size() << " cluster(s) selected\n";

    if (cfg.output_fmt == "json") {
        std::cout << clusters_to_json(top, cfg.show_samples);
    } else {
        std::cout << format_text_report(top, cfg.show_samples);
    }

    return top.empty() ? 0 : 1;
}
