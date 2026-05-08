# LogTriage++

A high-performance log analysis tool written in C++17. Reads structured log files, parses and normalizes log messages, groups them into clusters by signature and component, and reports the top recurring issues ranked by severity and frequency.

## Features

- Parallel log parsing via `std::thread` with configurable thread count
- Message normalization — strips IPs, UUIDs, numbers, paths, and quoted strings to produce stable cluster keys
- Clustering by `component|normalized_message` signature
- Severity-based filtering (`any`, `warn`, `error`)
- Text and JSON output formats with optional sample log lines per cluster
- Reads from files or stdin
- Non-zero exit code (1) when issues are found — suitable for CI pipelines
- Google Test unit test suite

## Requirements

- CMake 3.14+
- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Internet access on first build (CMake fetches GoogleTest automatically)

## Build

```bash
cmake -S . -B build
cmake --build build -j4
```

## Usage

```bash
# Analyze one or more log files
./build/logtriage --file app.log --file infra.log

# Filter to only ERROR-level clusters
./build/logtriage --file app.log --issue-level error

# JSON output with sample lines
./build/logtriage --file app.log --output json --samples

# Read from stdin
cat app.log | ./build/logtriage

# Show top 5 clusters with at least 3 occurrences
./build/logtriage --file app.log --top 5 --min-count 3
```

### Options

| Flag | Default | Description |
|------|---------|-------------|
| `--file <path>` | — | Log file to parse (repeatable) |
| `--threads <n>` | 4 | Worker threads |
| `--chunk-size <n>` | 500 | Lines per thread chunk |
| `--top <n>` | 10 | Max clusters to show |
| `--min-count <n>` | 2 | Min occurrences to include |
| `--issue-level <level>` | `any` | `any` \| `warn` \| `error` |
| `--output <fmt>` | `text` | `text` \| `json` |
| `--samples` | off | Show sample log lines per cluster |
| `--max-samples <n>` | 3 | Max samples per cluster |
| `--verbose` | off | Print progress to stderr |

### Exit Codes

| Code | Meaning |
|------|---------|
| 0 | No matching clusters found |
| 1 | One or more clusters found |
| 2 | Invalid arguments or input error |

## Log Format

LogTriage++ expects structured log lines in the form:

```
<timestamp> <LEVEL> <component> <message>
```

Example:
```
2026-01-25T12:34:56Z ERROR auth Login failed for user admin
2026-01-25T12:34:57Z WARN  db   Slow query took 3200ms on table users
```

Lines that do not match this pattern are still ingested as `UNKNOWN` level/component.

## Run Tests

```bash
./build/logtriage_tests
# or
cd build && ctest -V
```

## Project Structure

```
LogTriage++/
├── src/
│   ├── model.hpp / model.cpp       # LogEvent, Cluster data structures
│   ├── constants.hpp               # Regex patterns for parsing and normalization
│   ├── parser.hpp / parser.cpp     # normalize_message, parse_line, parse_chunk
│   ├── cluster.hpp / cluster.cpp   # build_clusters, select_top_clusters
│   ├── output.hpp / output.cpp     # Text and JSON formatters
│   ├── io.hpp / io.cpp             # File and stdin readers
│   └── main.cpp                    # CLI entry point
├── tests/
│   └── test_core.cpp               # Google Test suite
├── CMakeLists.txt
└── README.md
```

## Architecture

Log lines are split into equal chunks and dispatched to N worker threads, each running `parse_chunk` independently with no shared mutable state. After all threads join, results are merged in the main thread and fed into `build_clusters`. This merge-after pattern eliminates the need for mutexes during parsing.

```
stdin/files → read_lines → [chunk 0] thread 0 → parse_chunk
                          → [chunk 1] thread 1 → parse_chunk  → merge → build_clusters → select_top → format
                          → [chunk N] thread N → parse_chunk
```
