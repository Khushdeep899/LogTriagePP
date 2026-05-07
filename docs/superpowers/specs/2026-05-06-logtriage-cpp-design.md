# LogTriage++ Design Spec
**Date:** 2026-05-06
**Status:** Approved

## Overview

A C++17 port of the Python LogTriage tool. Parses log files in parallel, normalizes messages, and clusters similar events by signature and severity. Built with CMake, uses `std::thread` for parallel parsing, and tested with Google Test.

The primary goal beyond functionality is **readability for a C++ beginner** — every non-obvious C++ pattern is annotated with a comment explaining it and its Python equivalent.

---

## Project Structure

```
LogTriage++/
├── CMakeLists.txt
├── src/
│   ├── model.hpp           # LogEvent + Cluster structs, SEVERITY_ORDER
│   ├── constants.hpp       # LOG_LINE_PATTERN + NORMALIZE_RULES
│   ├── parser.hpp          # declarations for normalize, parse_line, parse_chunk
│   ├── parser.cpp          # implementations
│   ├── cluster.hpp         # declarations for build_clusters, select_top_clusters
│   ├── cluster.cpp         # implementations
│   ├── output.hpp          # declarations for format_text_report, clusters_to_json
│   ├── output.cpp          # implementations
│   ├── io.hpp              # declarations for read_lines_from_files
│   ├── io.cpp              # implementations
│   └── main.cpp            # CLI entry point, argument parsing, orchestration
└── tests/
    └── test_core.cpp       # 4 Google Test unit tests
```

One `.hpp`/`.cpp` pair per Python source module. Direct 1:1 mapping so learner can compare.

---

## Data Model (`model.hpp`)

### `LogEvent` struct
| Field | Type | Python equivalent |
|---|---|---|
| `timestamp` | `std::optional<std::string>` | `Optional[str]` |
| `level` | `std::string` | `str` |
| `component` | `std::string` | `str` |
| `message` | `std::string` | `str` |
| `normalized` | `std::string` | `str` |

### `Cluster` struct
| Field | Type | Python equivalent |
|---|---|---|
| `count` | `int` | `int` |
| `first_ts` | `std::optional<std::string>` | `Optional[str]` |
| `last_ts` | `std::optional<std::string>` | `Optional[str]` |
| `level_counts` | `std::map<std::string, int>` | `Counter` |
| `samples` | `std::vector<std::string>` | `list[str]` |

Methods: `max_severity() const`, `top_level() const`

### `SEVERITY_ORDER`
`std::map<std::string, int>` — TRACE=10, DEBUG=20, INFO=30, WARN/WARNING=40, ERROR=50, CRITICAL=60, FATAL=70

Free function: `int severity_score(const std::string& level)`

---

## Constants (`constants.hpp`)

- `LOG_LINE_PATTERN` — `std::string` raw string literal matching: optional ISO 8601 timestamp, bracketed or bare log level, optional component, message body
- `NORMALIZE_RULES` — `std::vector<std::pair<std::string, std::string>>` of `{pattern, replacement}` pairs covering: UUID, hex, IP, quoted strings, file paths, numbers, extra whitespace

---

## Parser (`parser.hpp` / `parser.cpp`)

### `normalize_message(const std::string& msg) -> std::string`
Applies each rule in `NORMALIZE_RULES` using `std::regex_replace`. Returns cleaned string.

### `parse_line(const std::string& line) -> std::optional<LogEvent>`
Uses `std::regex` + `std::smatch` against `LOG_LINE_PATTERN`. Returns `std::nullopt` for empty/unmatched lines. Unmatched lines get level `"UNKNOWN"` and full line as message (same as Python fallback).

### `parse_chunk(const std::vector<std::string>& lines, int start, int end) -> std::vector<LogEvent>`
Processes lines `[start, end)`, calls `parse_line` on each, skips nullopt results.

---

## Cluster (`cluster.hpp` / `cluster.cpp`)

### `build_clusters(const std::vector<LogEvent>& events, int max_samples) -> std::unordered_map<std::string, Cluster>`
Groups events by key `component + "|" + normalized`. Updates count, timestamps, level_counts, samples per cluster.

### `select_top_clusters(const std::unordered_map<std::string, Cluster>&, int top_n, int min_count, int min_severity) -> std::vector<std::pair<std::string, Cluster>>`
Filters by `min_count` and `min_severity`, sorts by severity descending then count descending, returns top `top_n`.

### Thread safety
Each thread calls `parse_chunk` on its non-overlapping slice of lines, returning a `std::vector<LogEvent>`. The main thread joins all threads, merges the per-thread vectors into one big vector, then calls `build_clusters` once. No mutex required — threads only write to their own local vector during parsing.

---

## Output (`output.hpp` / `output.cpp`)

### `format_text_report(const std::vector<std::pair<std::string, Cluster>>&, bool show_samples) -> std::string`
Mirrors Python's text report: numbered entries, component/signature, count, level distribution, timestamp window, optional samples.

### `clusters_to_json(const std::vector<std::pair<std::string, Cluster>>&, bool include_samples) -> std::string`
Hand-builds JSON string — no external library. Produces same structure as Python's JSON output: `generated_at`, array of cluster objects.

---

## IO (`io.hpp` / `io.cpp`)

### `read_lines_from_files(const std::vector<std::string>& paths) -> std::vector<std::string>`
Opens each file with `std::ifstream`, reads line by line with `std::getline`, returns all lines. Prints warning to `std::cerr` for unreadable files (same as Python behavior).

### Stdin support
If no `--file` flags given, reads from `std::cin` line by line.

---

## CLI (`main.cpp`)

Manual `argv[]` loop parser supporting all Python CLI flags:

| Flag | Type | Default |
|---|---|---|
| `--file <path>` | repeatable | required |
| `--threads <n>` | int | 4 |
| `--chunk-size <n>` | int | 500 |
| `--top <n>` | int | 10 |
| `--min-count <n>` | int | 2 |
| `--issue-level <any\|warn\|error>` | string | `"any"` |
| `--output <text\|json>` | string | `"text"` |
| `--samples` | bool flag | false |
| `--max-samples <n>` | int | 3 |
| `--verbose` | bool flag | false |
| `--run-tests` | bool flag | false |

Exit codes: 0 (no issues), 1 (issues found), 2 (invalid input) — same as Python.

### Orchestration flow in `main()`
1. Parse args → validate
2. If `--run-tests`: run gtest and exit
3. Read lines (files or stdin)
4. Split into chunks
5. Spawn N `std::thread`s, each calling `parse_chunk` on its slice
6. `thread.join()` all threads
7. Merge partial event vectors
8. `build_clusters()` → `select_top_clusters()`
9. Print via `format_text_report` or `clusters_to_json`
10. Return exit code

---

## Tests (`tests/test_core.cpp`)

4 Google Tests mirroring Python's `test_core.py`:

| Test | What it checks |
|---|---|
| `test_normalize_masks_uuid_ip_num` | `normalize_message()` replaces UUID, IP, number with placeholders |
| `test_parse_line_basic` | `parse_line()` extracts level, component, message from a timestamped line |
| `test_cluster_counts` | `build_clusters()` groups 2 identical messages into 1 cluster with count=2 |
| `test_select_top_filters_by_severity` | `select_top_clusters()` excludes clusters below severity threshold |

---

## Build System (`CMakeLists.txt`)

- CMake minimum: 3.14 (needed for `FetchContent`)
- C++ standard: 17
- `FetchContent` pulls Google Test from GitHub
- Two targets: `logtriage` (executable) and `logtriage_tests` (test binary)
- Sources in `src/`, tests in `tests/`

---

## Comment Style

Every non-obvious C++ pattern gets a comment in format:
```cpp
// C++ concept: what it is
// Python equivalent: what you'd write in Python
```

Example:
```cpp
// std::optional<T> means this value might not exist — like "str | None" in Python
std::optional<std::string> timestamp;
```

---

## What This Project Teaches

Working through this codebase exposes you to:
- Structs as data containers (vs Python dataclasses)
- Header/implementation file split
- `std::vector`, `std::map`, `std::unordered_map`
- `std::optional` for nullable values
- `std::regex` and raw string literals
- `std::thread` and the merge-after threading pattern
- `std::ifstream` for file I/O
- CMake project structure
- Google Test basics
