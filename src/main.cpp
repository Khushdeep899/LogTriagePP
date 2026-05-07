// src/main.cpp
//
// Entry point. Parses CLI arguments, reads log lines, spawns threads to parse
// them in parallel, merges results, clusters events, and prints the report.
//
// This file is the C++ equivalent of Python's cli.py.

#include <iostream>
#include <string>
#include <vector>
#include <thread>      // std::thread — like Python's threading.Thread
#include <stdexcept>   // std::runtime_error — like Python's RuntimeError
#include <algorithm>   // std::min, std::max

#include "model.hpp"
#include "io.hpp"
#include "parser.hpp"
#include "cluster.hpp"
#include "output.hpp"

// Config holds all parsed CLI arguments
// Like Python's argparse Namespace object — a plain bundle of settings
struct Config {
    std::vector<std::string> files;           // --file (repeatable)
    int         threads      = 4;             // --threads
    int         chunk_size   = 500;           // --chunk-size
    int         top_n        = 10;            // --top
    int         min_count    = 2;             // --min-count
    std::string issue_level  = "any";         // --issue-level
    std::string output_fmt   = "text";        // --output
    bool        show_samples = false;         // --samples
    int         max_samples  = 3;             // --max-samples
    bool        verbose      = false;         // --verbose
    bool        run_tests    = false;         // --run-tests
};

// Maps "--issue-level" string to a minimum severity number
// Same logic as Python's issue_min_severity()
int issue_min_severity(const std::string& level) {
    if (level == "warn")  return 40;   // WARN and above
    if (level == "error") return 50;   // ERROR and above
    return 0;                          // "any" — no severity filter
}

// Print usage instructions to stderr
void print_usage(const char* prog) {
    std::cerr
        << "Usage: " << prog << " [options]\n"
        << "  --file <path>           Log file to parse (can be repeated)\n"
        << "  --threads <n>           Worker threads (default: 4)\n"
        << "  --chunk-size <n>        Lines per thread chunk (default: 500)\n"
        << "  --top <n>               Max clusters to show (default: 10)\n"
        << "  --min-count <n>         Min occurrences to include (default: 2)\n"
        << "  --issue-level <level>   any|warn|error (default: any)\n"
        << "  --output <fmt>          text|json (default: text)\n"
        << "  --samples               Show sample log lines per cluster\n"
        << "  --max-samples <n>       Max samples to store per cluster (default: 3)\n"
        << "  --verbose               Print progress information to stderr\n"
        << "  --run-tests             Show command to run the unit test binary\n";
}

// Parse command-line arguments into a Config struct
//
// argc = count of arguments (like len(sys.argv) in Python)
// argv = array of C strings (like sys.argv — argv[0] is the program name)
Config parse_args(int argc, char* argv[]) {
    Config cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // Lambda that safely fetches the next argument value
        // Captures 'i' and 'argc' by reference so it can advance the index
        // Like Python's argparse consuming the next token after a flag
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
            std::exit(2);  // exit code 2 = bad input (same as Python)
        }
    }

    return cfg;
}

int main(int argc, char* argv[]) {
    // ── Parse arguments ─────────────────────────────────────────────────────
    // try/catch is like Python's try/except — catches exceptions
    Config cfg;
    try {
        cfg = parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        print_usage(argv[0]);
        return 2;
    }

    // Validate string options (numbers are validated by std::stoi above)
    if (cfg.issue_level != "any" && cfg.issue_level != "warn" && cfg.issue_level != "error") {
        std::cerr << "Invalid --issue-level: " << cfg.issue_level
                  << " (must be: any, warn, error)\n";
        return 2;
    }
    if (cfg.output_fmt != "text" && cfg.output_fmt != "json") {
        std::cerr << "Invalid --output: " << cfg.output_fmt
                  << " (must be: text, json)\n";
        return 2;
    }

    // --run-tests: in C++, tests live in a separate compiled binary
    // (Google Test runs as its own executable, not embedded in the main binary)
    if (cfg.run_tests) {
        std::cout << "To run unit tests:\n"
                  << "  cd build && ./logtriage_tests\n"
                  << "  (or: cd build && ctest -V  for verbose output)\n";
        return 0;
    }

    // ── Step 1: Read all log lines ───────────────────────────────────────────
    std::vector<std::string> lines;
    if (!cfg.files.empty()) {
        lines = read_lines_from_files(cfg.files);
    } else {
        // No --file given: read from stdin — like "cat app.log | ./logtriage"
        lines = read_lines_from_stdin();
    }

    if (cfg.verbose)
        std::cerr << "[verbose] Read " << lines.size() << " lines\n";

    if (lines.empty()) {
        std::cerr << "No input lines to process.\n";
        return 0;
    }

    // ── Step 2: Divide work and spawn threads ────────────────────────────────
    int total     = static_cast<int>(lines.size());
    // Don't create more threads than there are chunks of work
    int n_chunks  = std::max(1, (total + cfg.chunk_size - 1) / cfg.chunk_size);
    int n_threads = std::min(cfg.threads, n_chunks);
    int chunk     = (total + n_threads - 1) / n_threads;  // lines per thread

    if (cfg.verbose)
        std::cerr << "[verbose] Spawning " << n_threads << " thread(s), "
                  << chunk << " lines each\n";

    // One result vector per thread — each thread writes only to its own slot
    // This is the "merge-after" pattern: no mutex needed during parsing
    std::vector<std::vector<LogEvent>> thread_results(n_threads);

    // std::vector<std::thread> holds all thread objects — like a list of threads
    std::vector<std::thread> threads;

    for (int t = 0; t < n_threads; ++t) {
        int start = t * chunk;
        int end   = std::min(start + chunk, total);

        // threads.emplace_back() creates a thread in-place — like list.append(Thread(...))
        // The lambda is the function the thread will run
        //
        // IMPORTANT: t, start, end are captured BY VALUE (no &)
        // If captured by reference, all threads would share the same variable
        // and by the time they run, the loop might have already incremented them.
        // Capturing by value gives each thread its own private copy.
        threads.emplace_back(
            [&lines, &thread_results, t, start, end]() {
                thread_results[t] = parse_chunk(lines, start, end);
            }
        );
    }

    // ── Step 3: Wait for all threads to finish ───────────────────────────────
    // .join() blocks until the thread completes — like thread.join() in Python
    // Must join every thread before accessing thread_results
    for (auto& th : threads) {
        th.join();
    }

    // ── Step 4: Merge per-thread event lists into one flat list ─────────────
    std::vector<LogEvent> all_events;
    for (const auto& part : thread_results) {
        // .insert(end, begin, end) appends a range — like Python's list.extend()
        all_events.insert(all_events.end(), part.begin(), part.end());
    }

    if (cfg.verbose)
        std::cerr << "[verbose] Parsed " << all_events.size() << " events\n";

    // ── Step 5: Cluster and filter ───────────────────────────────────────────
    auto clusters = build_clusters(all_events, cfg.max_samples);
    int  min_sev  = issue_min_severity(cfg.issue_level);
    auto top      = select_top_clusters(clusters, cfg.top_n, cfg.min_count, min_sev);

    if (cfg.verbose)
        std::cerr << "[verbose] " << top.size() << " cluster(s) selected\n";

    // ── Step 6: Print output ─────────────────────────────────────────────────
    if (cfg.output_fmt == "json") {
        std::cout << clusters_to_json(top, cfg.show_samples);
    } else {
        std::cout << format_text_report(top, cfg.show_samples);
    }

    // Exit code: 0 = no issues found, 1 = issues found (same convention as Python)
    // Lets CI pipelines detect problems: "run logtriage || alert"
    return top.empty() ? 0 : 1;
}
