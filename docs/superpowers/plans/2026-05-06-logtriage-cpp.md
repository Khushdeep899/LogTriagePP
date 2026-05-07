# LogTriage++ Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build LogTriage++ — a beginner-readable C++17 port of a Python log triage tool that parses log files in parallel, normalizes messages, and clusters events by signature and severity.

**Architecture:** Mirror the Python module structure 1:1 (model, constants, parser, cluster, output, io, main). Use std::thread for parallel parsing with a merge-after pattern — each thread parses its own slice of lines into a local vector, then the main thread merges all vectors before clustering. No mutex needed during parsing.

**Tech Stack:** C++17, CMake 3.14+, Google Test 1.14.0 (via FetchContent), std::thread, std::regex

---

## File Map

| File | Responsibility |
|---|---|
| `CMakeLists.txt` | Build config, FetchContent for gtest, two targets |
| `src/model.hpp` | LogEvent + Cluster structs, SEVERITY_ORDER |
| `src/model.cpp` | severity_score(), Cluster::max_severity(), Cluster::top_level() |
| `src/constants.hpp` | LOG_LINE_PATTERN + NORMALIZE_RULES |
| `src/parser.hpp` | Declarations: normalize_message, parse_line, parse_chunk |
| `src/parser.cpp` | Implementations |
| `src/cluster.hpp` | Declarations: build_clusters, select_top_clusters |
| `src/cluster.cpp` | Implementations |
| `src/output.hpp` | Declarations: format_text_report, clusters_to_json |
| `src/output.cpp` | Implementations |
| `src/io.hpp` | Declarations: read_lines_from_files, read_lines_from_stdin |
| `src/io.cpp` | Implementations |
| `src/main.cpp` | CLI arg parsing + threading orchestration |
| `tests/test_core.cpp` | 4 Google Test unit tests |

---

### Task 1: Project scaffold — directories, CMakeLists.txt, stub files, first build

**Files:**
- Create: `CMakeLists.txt`
- Create: `src/model.cpp` (stub)
- Create: `src/parser.cpp` (stub)
- Create: `src/cluster.cpp` (stub)
- Create: `src/output.cpp` (stub)
- Create: `src/io.cpp` (stub)
- Create: `src/main.cpp` (stub)
- Create: `tests/test_core.cpp` (empty shell)

- [ ] **Step 1: Create directory structure**

```bash
mkdir -p src tests build
```

- [ ] **Step 2: Write CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.14)
project(LogTriagePP CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FetchContent downloads dependencies automatically — like pip install, but for C++
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

# Sources shared between the main binary and the test binary
set(LT_SOURCES
    src/model.cpp
    src/parser.cpp
    src/cluster.cpp
    src/output.cpp
    src/io.cpp
)

# Main executable: logtriage
add_executable(logtriage ${LT_SOURCES} src/main.cpp)
target_include_directories(logtriage PRIVATE src)

# Test binary: logtriage_tests
add_executable(logtriage_tests ${LT_SOURCES} tests/test_core.cpp)
target_include_directories(logtriage_tests PRIVATE src)
target_link_libraries(logtriage_tests gtest_main)

include(GoogleTest)
gtest_discover_tests(logtriage_tests)
```

- [ ] **Step 3: Write stub source files**

`src/model.cpp`:
```cpp
#include "model.hpp"

int severity_score(const std::string& level) { return 0; }
int Cluster::max_severity() const { return 0; }
std::string Cluster::top_level() const { return "UNKNOWN"; }
```

`src/parser.cpp`:
```cpp
#include "parser.hpp"

std::string normalize_message(const std::string& msg) { return msg; }
std::optional<LogEvent> parse_line(const std::string& line) { return std::nullopt; }
std::vector<LogEvent> parse_chunk(const std::vector<std::string>& lines, int start, int end) { return {}; }
```

`src/cluster.cpp`:
```cpp
#include "cluster.hpp"

std::unordered_map<std::string, Cluster> build_clusters(const std::vector<LogEvent>& events, int max_samples) { return {}; }
std::vector<std::pair<std::string, Cluster>> select_top_clusters(const std::unordered_map<std::string, Cluster>& clusters, int top_n, int min_count, int min_severity) { return {}; }
```

`src/output.cpp`:
```cpp
#include "output.hpp"

std::string format_text_report(const std::vector<std::pair<std::string, Cluster>>& clusters, bool show_samples) { return ""; }
std::string clusters_to_json(const std::vector<std::pair<std::string, Cluster>>& clusters, bool include_samples) { return "{}"; }
```

`src/io.cpp`:
```cpp
#include "io.hpp"

std::vector<std::string> read_lines_from_files(const std::vector<std::string>& paths) { return {}; }
std::vector<std::string> read_lines_from_stdin() { return {}; }
```

`src/main.cpp`:
```cpp
int main() { return 0; }
```

`tests/test_core.cpp`:
```cpp
#include <gtest/gtest.h>
#include "model.hpp"
#include "parser.hpp"
#include "cluster.hpp"
// Tests added in Tasks 4-7
```

- [ ] **Step 4: Write stub header files**

`src/model.hpp`:
```cpp
#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>

struct LogEvent {
    std::optional<std::string> timestamp;
    std::string level;
    std::string component;
    std::string message;
    std::string normalized;
};

struct Cluster {
    int count = 0;
    std::optional<std::string> first_ts;
    std::optional<std::string> last_ts;
    std::map<std::string, int> level_counts;
    std::vector<std::string> samples;
    int max_severity() const;
    std::string top_level() const;
};

inline const std::map<std::string, int> SEVERITY_ORDER = {
    {"TRACE", 10}, {"DEBUG", 20}, {"INFO", 30},
    {"WARN", 40},  {"WARNING", 40}, {"ERROR", 50},
    {"CRITICAL", 60}, {"FATAL", 70}
};

int severity_score(const std::string& level);
```

`src/constants.hpp`:
```cpp
#pragma once
#include <string>
#include <vector>
#include <utility>

inline const std::string LOG_LINE_PATTERN =
    R"(^(?:(\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:?\d{2})?)\s+)?)"
    R"((?:\[?)([A-Z]+)(?:\]?)\s+)"
    R"((?:\[?([a-zA-Z0-9._-]+)\]?\s+)?)"
    R"((.+)$)";

inline const std::vector<std::pair<std::string, std::string>> NORMALIZE_RULES = {
    {R"([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})", "<UUID>"},
    {R"(0x[0-9a-fA-F]+)", "<HEX>"},
    {R"(\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b)", "<IP>"},
    {R"("([^"]*)")", "<STR>"},
    {R"(/[^\s]+)", "<PATH>"},
    {R"(\b\d+\b)", "<NUM>"},
    {R"(\s{2,})", " "},
};
```

`src/parser.hpp`:
```cpp
#pragma once
#include <string>
#include <vector>
#include <optional>
#include "model.hpp"

std::string normalize_message(const std::string& msg);
std::optional<LogEvent> parse_line(const std::string& line);
std::vector<LogEvent> parse_chunk(const std::vector<std::string>& lines, int start, int end);
```

`src/cluster.hpp`:
```cpp
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include "model.hpp"

std::unordered_map<std::string, Cluster> build_clusters(
    const std::vector<LogEvent>& events, int max_samples = 5);

std::vector<std::pair<std::string, Cluster>> select_top_clusters(
    const std::unordered_map<std::string, Cluster>& clusters,
    int top_n = 10, int min_count = 2, int min_severity = 0);
```

`src/output.hpp`:
```cpp
#pragma once
#include <string>
#include <vector>
#include <utility>
#include "model.hpp"

std::string format_text_report(
    const std::vector<std::pair<std::string, Cluster>>& clusters,
    bool show_samples = false);

std::string clusters_to_json(
    const std::vector<std::pair<std::string, Cluster>>& clusters,
    bool include_samples = false);
```

`src/io.hpp`:
```cpp
#pragma once
#include <string>
#include <vector>

std::vector<std::string> read_lines_from_files(const std::vector<std::string>& paths);
std::vector<std::string> read_lines_from_stdin();
```

- [ ] **Step 5: Configure and build**

```bash
cd build && cmake .. && cmake --build . -j4
```

Expected: Build succeeds. `logtriage` and `logtriage_tests` binaries created.

- [ ] **Step 6: Run tests (expect 0 tests, no failures)**

```bash
cd build && ./logtriage_tests
```

Expected output: `[==========] 0 tests from 0 test suites ran.`

---

### Task 2: model.cpp — severity logic and Cluster methods

**Files:**
- Modify: `src/model.cpp`

- [ ] **Step 1: Replace stub with real implementation**

```cpp
// src/model.cpp
//
// Implements the severity scoring and Cluster helper methods.
// No includes needed beyond model.hpp — all types are defined there.

#include "model.hpp"

// severity_score: look up a level name in SEVERITY_ORDER
// The '&' means we pass by reference — no copy made, like Python passing an object
// 'const' means we promise not to modify the string
int severity_score(const std::string& level) {
    // .find() returns an iterator — think of it as a pointer to the found element
    // .end() means "not found" — like checking "if key not in dict" in Python
    auto it = SEVERITY_ORDER.find(level);
    if (it == SEVERITY_ORDER.end()) return 0;
    // it->second is the value in the key/value pair (it->first is the key)
    return it->second;
}

// max_severity: highest severity score among all levels seen in this cluster
// 'const' here means this method doesn't modify 'this' (the Cluster object)
int Cluster::max_severity() const {
    int max = 0;
    // Structured binding: [level, count] unpacks each map pair
    // Like "for level, count in self.level_counts.items():" in Python
    for (const auto& [level, count] : level_counts) {
        int s = severity_score(level);
        if (s > max) max = s;
    }
    return max;
}

// top_level: most common level, ties broken by severity (higher wins)
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
```

- [ ] **Step 2: Build to confirm no errors**

```bash
cd build && cmake --build . -j4
```

Expected: Build succeeds.

---

### Task 3: TDD — normalize_message

**Files:**
- Modify: `tests/test_core.cpp`
- Modify: `src/parser.cpp`

- [ ] **Step 1: Add failing test to test_core.cpp**

Replace the contents of `tests/test_core.cpp` with:

```cpp
#include <gtest/gtest.h>
#include "model.hpp"
#include "parser.hpp"
#include "cluster.hpp"

// ── Test 1: normalize_message ────────────────────────────────────────────────
//
// normalize_message should replace variable parts (UUIDs, IPs, numbers)
// with fixed placeholders so that two messages that differ only in those
// values produce the same normalized string — making them cluster together.

TEST(NormalizeTest, MasksUuidIpNum) {
    std::string msg = "conn from 10.0.0.1 id=3f2504e0-4f89-11d3-9a0c-0305e82c3301 took 123ms";
    std::string out = normalize_message(msg);

    // Original values must be gone
    // std::string::npos is the "not found" sentinel returned by .find()
    EXPECT_EQ(out.find("10.0.0.1"),  std::string::npos);
    EXPECT_EQ(out.find("3f2504e0"),  std::string::npos);
    EXPECT_EQ(out.find("123"),        std::string::npos);

    // Placeholders must be present
    // EXPECT_NE means "expect not equal" — i.e., the substring WAS found
    EXPECT_NE(out.find("<IP>"),   std::string::npos);
    EXPECT_NE(out.find("<UUID>"), std::string::npos);
    EXPECT_NE(out.find("<NUM>"),  std::string::npos);
}
```

- [ ] **Step 2: Build and run — confirm test FAILS**

```bash
cd build && cmake --build . -j4 && ./logtriage_tests
```

Expected: Test runs and FAILS — the stub returns the input unchanged so `10.0.0.1` is still present.

- [ ] **Step 3: Implement normalize_message in parser.cpp**

```cpp
// src/parser.cpp

#include "parser.hpp"
#include "constants.hpp"
#include <regex>

// normalize_message: apply each substitution rule in order
// std::regex_replace is like Python's re.sub(pattern, replacement, text)
std::string normalize_message(const std::string& msg) {
    std::string result = msg;

    // Range-based for loop — like "for pattern, replacement in NORMALIZE_RULES:"
    // 'auto& [pattern, replacement]' is a structured binding — unpacks the pair
    for (const auto& [pattern, replacement] : NORMALIZE_RULES) {
        // std::regex compiles the pattern string into a regex object
        result = std::regex_replace(result, std::regex(pattern), replacement);
    }

    return result;
}

// Stubs — implemented in later tasks
std::optional<LogEvent> parse_line(const std::string& line) { return std::nullopt; }
std::vector<LogEvent> parse_chunk(const std::vector<std::string>& lines, int start, int end) { return {}; }
```

- [ ] **Step 4: Build and run — confirm test PASSES**

```bash
cd build && cmake --build . -j4 && ./logtriage_tests
```

Expected: `[ PASSED ] 1 test.`

- [ ] **Step 5: Commit**

```bash
git add src/parser.cpp tests/test_core.cpp
git commit -m "feat: implement normalize_message with regex substitution rules"
```

---

### Task 4: TDD — parse_line and parse_chunk

**Files:**
- Modify: `tests/test_core.cpp`
- Modify: `src/parser.cpp`

- [ ] **Step 1: Add failing test for parse_line**

Append to `tests/test_core.cpp` (after the closing brace of NormalizeTest):

```cpp
// ── Test 2: parse_line ───────────────────────────────────────────────────────
//
// parse_line should extract structured fields from a log line.
// The result is wrapped in std::optional — we must check .has_value() before use.

TEST(ParseLineTest, ExtractsFields) {
    std::string line = "2026-01-25T12:34:56Z ERROR auth Login failed for user admin";
    auto result = parse_line(line);

    // ASSERT_TRUE stops the test immediately if false — use for preconditions
    // EXPECT_* continues even on failure — use for individual assertions
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->level,     "ERROR");
    EXPECT_EQ(result->component, "auth");
    // ->message accesses the field through the optional (sugar for .value().message)
    EXPECT_NE(result->message.find("Login failed"), std::string::npos);
}
```

- [ ] **Step 2: Build and run — confirm new test FAILS**

```bash
cd build && cmake --build . -j4 && ./logtriage_tests
```

Expected: `ParseLineTest.ExtractsFields` FAILS (stub returns nullopt).

- [ ] **Step 3: Implement parse_line and parse_chunk in parser.cpp**

Replace the stub implementations in `src/parser.cpp` (keep normalize_message as-is, replace only the two stubs at the bottom):

```cpp
std::optional<LogEvent> parse_line(const std::string& line) {
    // Skip blank lines
    if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
        return std::nullopt;  // like Python's "return None"
    }

    // 'static' here means the regex is compiled only once, not on every call
    // Compiling a regex is expensive — this is like Python's re.compile() at module level
    static const std::regex log_re(LOG_LINE_PATTERN);

    // std::smatch holds capture groups — like Python's match.groups()
    std::smatch m;
    LogEvent event;

    if (std::regex_search(line, m, log_re)) {
        // m[1] = timestamp (group 1), m[2] = level, m[3] = component, m[4] = message
        // .matched tells us if the group participated in the match
        if (m[1].matched) event.timestamp = m[1].str();
        event.level     = m[2].matched ? m[2].str() : "UNKNOWN";
        event.component = m[3].matched ? m[3].str() : "UNKNOWN";
        event.message   = m[4].matched ? m[4].str() : line;
    } else {
        // Line didn't match — treat whole line as the message (same as Python fallback)
        event.level     = "UNKNOWN";
        event.component = "UNKNOWN";
        event.message   = line;
    }

    event.normalized = normalize_message(event.message);
    return event;
}

std::vector<LogEvent> parse_chunk(const std::vector<std::string>& lines,
                                   int start, int end) {
    std::vector<LogEvent> events;

    // static_cast<int>(...) converts size_t (unsigned) to int — avoids compiler warning
    for (int i = start; i < end && i < static_cast<int>(lines.size()); ++i) {
        auto opt = parse_line(lines[i]);
        if (opt.has_value()) {
            // .push_back() appends to the vector — like Python's list.append()
            events.push_back(opt.value());
        }
    }

    return events;
}
```

- [ ] **Step 4: Build and run — confirm both tests PASS**

```bash
cd build && cmake --build . -j4 && ./logtriage_tests
```

Expected: `[ PASSED ] 2 tests.`

- [ ] **Step 5: Commit**

```bash
git add src/parser.cpp tests/test_core.cpp
git commit -m "feat: implement parse_line and parse_chunk with regex extraction"
```

---

### Task 5: TDD — build_clusters

**Files:**
- Modify: `tests/test_core.cpp`
- Modify: `src/cluster.cpp`

- [ ] **Step 1: Add failing test**

Append to `tests/test_core.cpp`:

```cpp
// ── Test 3: build_clusters ───────────────────────────────────────────────────
//
// Two events with the same component and normalized message should land in
// one cluster with count=2.

TEST(ClusterTest, GroupsIdenticalSignatures) {
    // Build two events that differ in raw message but share normalized form
    LogEvent e1;
    e1.level = "ERROR"; e1.component = "db";
    e1.message = "connection timeout";
    e1.normalized = "connection timeout";  // already normalized

    LogEvent e2;
    e2.level = "ERROR"; e2.component = "db";
    e2.message = "connection timeout after 3000ms";
    e2.normalized = "connection timeout";  // same normalized form as e1

    auto clusters = build_clusters({e1, e2}, 5);

    // 1u = unsigned int literal — avoids signed/unsigned comparison warning
    ASSERT_EQ(clusters.size(), 1u);
    EXPECT_EQ(clusters.begin()->second.count, 2);
}
```

- [ ] **Step 2: Build and run — confirm new test FAILS**

```bash
cd build && cmake --build . -j4 && ./logtriage_tests
```

Expected: `ClusterTest.GroupsIdenticalSignatures` FAILS (stub returns empty map).

- [ ] **Step 3: Implement build_clusters in cluster.cpp**

```cpp
// src/cluster.cpp

#include "cluster.hpp"
#include "model.hpp"
#include <algorithm>

// Helper: merge one event into an existing cluster
// Takes cluster by reference (&) so changes persist — like passing a mutable object
static void update_cluster(Cluster& cluster, const LogEvent& event, int max_samples) {
    cluster.count++;

    if (event.timestamp.has_value()) {
        if (!cluster.first_ts.has_value()) cluster.first_ts = event.timestamp;
        cluster.last_ts = event.timestamp;
    }

    // operator[] on a map creates the entry with value 0 if missing
    // Same as Python's collections.defaultdict(int)
    cluster.level_counts[event.level]++;

    if (static_cast<int>(cluster.samples.size()) < max_samples) {
        cluster.samples.push_back(event.message);
    }
}

std::unordered_map<std::string, Cluster> build_clusters(
    const std::vector<LogEvent>& events, int max_samples)
{
    // std::unordered_map is like Python's dict — O(1) average lookup by key
    std::unordered_map<std::string, Cluster> clusters;

    for (const auto& event : events) {
        // Key = "component|normalized" — same grouping logic as Python
        std::string key = event.component + "|" + event.normalized;

        // clusters[key] creates an empty Cluster if key doesn't exist
        // Then update_cluster adds this event's data into it
        update_cluster(clusters[key], event, max_samples);
    }

    return clusters;
}

// Stub — implemented in next task
std::vector<std::pair<std::string, Cluster>> select_top_clusters(
    const std::unordered_map<std::string, Cluster>& clusters,
    int top_n, int min_count, int min_severity) { return {}; }
```

- [ ] **Step 4: Build and run — confirm 3 tests PASS**

```bash
cd build && cmake --build . -j4 && ./logtriage_tests
```

Expected: `[ PASSED ] 3 tests.`

- [ ] **Step 5: Commit**

```bash
git add src/cluster.cpp tests/test_core.cpp
git commit -m "feat: implement build_clusters with unordered_map grouping"
```

---

### Task 6: TDD — select_top_clusters

**Files:**
- Modify: `tests/test_core.cpp`
- Modify: `src/cluster.cpp`

- [ ] **Step 1: Add failing test**

Append to `tests/test_core.cpp`:

```cpp
// ── Test 4: select_top_clusters ──────────────────────────────────────────────
//
// Given an INFO cluster (severity 30) and an ERROR cluster (severity 50),
// filtering with min_severity=40 should return only the ERROR cluster.

TEST(SelectTopTest, FiltersBySeverity) {
    std::unordered_map<std::string, Cluster> clusters;

    Cluster info_c;
    info_c.count = 5;
    info_c.level_counts["INFO"] = 5;   // INFO = severity 30
    clusters["svc|info msg"] = info_c;

    Cluster err_c;
    err_c.count = 3;
    err_c.level_counts["ERROR"] = 3;  // ERROR = severity 50
    clusters["svc|err msg"] = err_c;

    // min_severity=40 means WARN+ only — INFO (30) must be excluded
    auto top = select_top_clusters(clusters, /*top_n=*/10, /*min_count=*/1, /*min_severity=*/40);

    ASSERT_EQ(top.size(), 1u);
    EXPECT_EQ(top[0].second.level_counts.at("ERROR"), 3);
}
```

- [ ] **Step 2: Build and run — confirm new test FAILS**

```bash
cd build && cmake --build . -j4 && ./logtriage_tests
```

Expected: `SelectTopTest.FiltersBySeverity` FAILS (stub returns empty vector).

- [ ] **Step 3: Implement select_top_clusters in cluster.cpp**

Replace the stub at the bottom of `src/cluster.cpp` with:

```cpp
std::vector<std::pair<std::string, Cluster>> select_top_clusters(
    const std::unordered_map<std::string, Cluster>& clusters,
    int top_n, int min_count, int min_severity)
{
    // Collect passing clusters into a sortable vector
    // std::pair<K,V> is a two-element struct — like a Python tuple (key, value)
    std::vector<std::pair<std::string, Cluster>> result;

    for (const auto& [key, cluster] : clusters) {
        if (cluster.count < min_count)            continue;
        if (cluster.max_severity() < min_severity) continue;
        result.push_back({key, cluster});
    }

    // std::sort with a lambda comparator
    // Lambda syntax: [captures](params) -> return_type { body }
    // The [] means no captures needed — it's a pure comparison function
    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) {
            int sev_a = a.second.max_severity();
            int sev_b = b.second.max_severity();
            if (sev_a != sev_b) return sev_a > sev_b;   // higher severity first
            return a.second.count > b.second.count;       // then higher count
        }
    );

    // Trim to top_n — like Python's result[:top_n]
    if (static_cast<int>(result.size()) > top_n) {
        result.resize(top_n);
    }

    return result;
}
```

- [ ] **Step 4: Build and run — confirm all 4 tests PASS**

```bash
cd build && cmake --build . -j4 && ./logtriage_tests
```

Expected: `[ PASSED ] 4 tests.`

- [ ] **Step 5: Commit**

```bash
git add src/cluster.cpp tests/test_core.cpp
git commit -m "feat: implement select_top_clusters with severity+count sort"
```

---

### Task 7: io.cpp — file and stdin reading

**Files:**
- Modify: `src/io.cpp`

- [ ] **Step 1: Implement io.cpp**

```cpp
// src/io.cpp

#include "io.hpp"
#include <fstream>    // std::ifstream — like Python's open()
#include <iostream>   // std::cin, std::cerr

std::vector<std::string> read_lines_from_files(const std::vector<std::string>& paths) {
    std::vector<std::string> lines;

    for (const auto& path : paths) {
        // std::ifstream opens a file for reading
        // Like Python's "f = open(path, 'r')"
        std::ifstream file(path);

        if (!file.is_open()) {
            // std::cerr is the error output stream — like Python's sys.stderr
            std::cerr << "Warning: could not open file: " << path << "\n";
            continue;  // skip this file, keep going
        }

        std::string line;
        // std::getline reads one line at a time, strips the newline character
        // The while condition is true as long as a line was successfully read
        // Like Python's "for line in f:"
        while (std::getline(file, line)) {
            lines.push_back(line);
        }

        // 'file' closes automatically here when it goes out of scope
        // This is RAII (Resource Acquisition Is Initialization) — C++'s version of
        // Python's "with open(...) as f:" — no need to call file.close() manually
    }

    return lines;
}

std::vector<std::string> read_lines_from_stdin() {
    std::vector<std::string> lines;
    std::string line;
    // std::cin is standard input — like Python's sys.stdin
    while (std::getline(std::cin, line)) {
        lines.push_back(line);
    }
    return lines;
}
```

- [ ] **Step 2: Build and confirm all 4 tests still pass**

```bash
cd build && cmake --build . -j4 && ./logtriage_tests
```

Expected: `[ PASSED ] 4 tests.`

- [ ] **Step 3: Commit**

```bash
git add src/io.cpp
git commit -m "feat: implement file and stdin reading with ifstream"
```

---

### Task 8: output.cpp — text and JSON formatting

**Files:**
- Modify: `src/output.cpp`

- [ ] **Step 1: Implement output.cpp**

```cpp
// src/output.cpp

#include "output.hpp"
#include <sstream>    // std::ostringstream — like Python's io.StringIO
#include <iomanip>    // std::put_time — for timestamp formatting
#include <chrono>     // std::chrono::system_clock — for current time
#include <ctime>      // std::gmtime
#include <algorithm>  // std::sort

// Helper: escape special characters in a string for JSON output
// Without this, a message containing '"' or '\' would break the JSON
static std::string json_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else                out += c;
    }
    return out;
}

std::string format_text_report(
    const std::vector<std::pair<std::string, Cluster>>& clusters,
    bool show_samples)
{
    // std::ostringstream builds a string incrementally
    // Like Python's: parts = []; parts.append("..."); return "".join(parts)
    std::ostringstream out;

    out << "\n=== LogTriage++ Report ===\n";
    out << "Found " << clusters.size() << " cluster(s)\n\n";

    int i = 1;
    for (const auto& [key, cluster] : clusters) {
        // Split "component|signature" key back into its two parts
        auto sep = key.find('|');
        std::string component = (sep != std::string::npos) ? key.substr(0, sep) : key;
        std::string signature = (sep != std::string::npos) ? key.substr(sep + 1) : key;

        out << "--- Cluster " << i++ << " ---\n";
        out << "  Count:     " << cluster.count        << "\n";
        out << "  Top Level: " << cluster.top_level()  << "\n";
        out << "  Component: " << component            << "\n";
        out << "  Signature: " << signature            << "\n";

        // .has_value() checks if the optional contains a value — like "is not None"
        // .value() unwraps it
        if (cluster.first_ts.has_value())
            out << "  First:     " << cluster.first_ts.value() << "\n";
        if (cluster.last_ts.has_value())
            out << "  Last:      " << cluster.last_ts.value()  << "\n";

        // Print level distribution sorted by count descending
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
    // Get current UTC time as ISO 8601 string
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ts;
    // std::put_time formats a time struct — like Python's strftime
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
        out << "      \"component\": \""  << json_escape(component)      << "\",\n";
        out << "      \"signature\": \""  << json_escape(signature)      << "\",\n";
        out << "      \"count\": "        << cluster.count               << ",\n";
        out << "      \"top_level\": \""  << cluster.top_level()         << "\",\n";
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
```

- [ ] **Step 2: Build and confirm all 4 tests still pass**

```bash
cd build && cmake --build . -j4 && ./logtriage_tests
```

Expected: `[ PASSED ] 4 tests.`

- [ ] **Step 3: Commit**

```bash
git add src/output.cpp
git commit -m "feat: implement text and JSON output formatters"
```

---

### Task 9: main.cpp — CLI argument parsing and parallel orchestration

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Implement main.cpp**

```cpp
// src/main.cpp
//
// Entry point. Parses CLI arguments, reads log lines, spawns threads,
// merges results, clusters events, and prints the report.
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
// Like Python's argparse Namespace object
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

// Maps "--issue-level" string to a minimum severity score
int issue_min_severity(const std::string& level) {
    if (level == "warn")  return 40;  // WARN and above
    if (level == "error") return 50;  // ERROR and above
    return 0;                         // "any" — no filter
}

void print_usage(const char* prog) {
    std::cerr
        << "Usage: " << prog << " [options]\n"
        << "  --file <path>           Log file to parse (can repeat)\n"
        << "  --threads <n>           Worker threads (default: 4)\n"
        << "  --chunk-size <n>        Lines per chunk (default: 500)\n"
        << "  --top <n>               Max clusters to show (default: 10)\n"
        << "  --min-count <n>         Min occurrences to include (default: 2)\n"
        << "  --issue-level <level>   any|warn|error (default: any)\n"
        << "  --output <fmt>          text|json (default: text)\n"
        << "  --samples               Show sample log lines per cluster\n"
        << "  --max-samples <n>       Max samples per cluster (default: 3)\n"
        << "  --verbose               Print progress to stderr\n"
        << "  --run-tests             Print command to run unit tests\n";
}

// Parse argv[] into a Config struct
// argc = number of arguments, argv = array of C strings (like Python's sys.argv)
Config parse_args(int argc, char* argv[]) {
    Config cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // Lambda that safely fetches the next argument
        // Captures i and argc by reference so it can advance the index
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
    // Parse arguments — exit 2 on bad input (same convention as Python version)
    Config cfg;
    try {
        cfg = parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        print_usage(argv[0]);
        return 2;
    }

    // Validate string options
    if (cfg.issue_level != "any" && cfg.issue_level != "warn" && cfg.issue_level != "error") {
        std::cerr << "Invalid --issue-level: " << cfg.issue_level << "\n";
        return 2;
    }
    if (cfg.output_fmt != "text" && cfg.output_fmt != "json") {
        std::cerr << "Invalid --output: " << cfg.output_fmt << "\n";
        return 2;
    }

    // --run-tests: in C++, tests are a separate binary compiled by CMake
    if (cfg.run_tests) {
        std::cout << "To run unit tests:\n"
                  << "  cd build && ./logtriage_tests\n"
                  << "  (or: cd build && ctest -V)\n";
        return 0;
    }

    // ── Step 1: Read lines ──────────────────────────────────────────────────
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

    // ── Step 2: Split work and spawn threads ────────────────────────────────
    int total    = static_cast<int>(lines.size());
    // Ensure at least 1 thread and don't create more threads than chunks
    int n_chunks = std::max(1, (total + cfg.chunk_size - 1) / cfg.chunk_size);
    int n_threads = std::min(cfg.threads, n_chunks);
    int chunk    = (total + n_threads - 1) / n_threads;  // lines per thread

    if (cfg.verbose)
        std::cerr << "[verbose] Spawning " << n_threads << " thread(s)\n";

    // One result vector per thread — threads write to their own slot, no mutex needed
    std::vector<std::vector<LogEvent>> thread_results(n_threads);

    // std::vector<std::thread> holds all thread objects
    std::vector<std::thread> threads;

    for (int t = 0; t < n_threads; ++t) {
        int start = t * chunk;
        int end   = std::min(start + chunk, total);

        // emplace_back constructs the thread in place — like list.append(Thread(...))
        // The lambda captures: lines and thread_results by reference (&),
        //                      t, start, end by value — critical! If captured by
        //                      reference, all threads would see the same final value.
        threads.emplace_back(
            [&lines, &thread_results, t, start, end]() {
                thread_results[t] = parse_chunk(lines, start, end);
            }
        );
    }

    // ── Step 3: Wait for all threads to finish ──────────────────────────────
    // .join() blocks until the thread completes — like thread.join() in Python
    for (auto& th : threads) {
        th.join();
    }

    // ── Step 4: Merge per-thread results into one flat vector ───────────────
    std::vector<LogEvent> all_events;
    for (const auto& part : thread_results) {
        // .insert() appends a range — like Python's list.extend()
        all_events.insert(all_events.end(), part.begin(), part.end());
    }

    if (cfg.verbose)
        std::cerr << "[verbose] Parsed " << all_events.size() << " events\n";

    // ── Step 5: Cluster and filter ──────────────────────────────────────────
    auto clusters = build_clusters(all_events, cfg.max_samples);
    int min_sev   = issue_min_severity(cfg.issue_level);
    auto top      = select_top_clusters(clusters, cfg.top_n, cfg.min_count, min_sev);

    if (cfg.verbose)
        std::cerr << "[verbose] " << top.size() << " cluster(s) selected\n";

    // ── Step 6: Output ──────────────────────────────────────────────────────
    if (cfg.output_fmt == "json") {
        std::cout << clusters_to_json(top, cfg.show_samples);
    } else {
        std::cout << format_text_report(top, cfg.show_samples);
    }

    // Exit code: 0 = no issues found, 1 = issues found (same as Python)
    return top.empty() ? 0 : 1;
}
```

- [ ] **Step 2: Build and confirm all 4 tests still pass**

```bash
cd build && cmake --build . -j4 && ./logtriage_tests
```

Expected: `[ PASSED ] 4 tests.` and `logtriage` binary built.

- [ ] **Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "feat: implement CLI argument parsing and parallel log orchestration"
```

---

### Task 10: Sample log files + integration test

**Files:**
- Create: `sample_app.log`
- Create: `sample_infrastructure.log`

- [ ] **Step 1: Create sample_app.log**

```
2026-01-25T10:15:32Z ERROR auth Failed login attempt for user admin from 192.168.1.100
2026-01-25T10:15:45Z ERROR payment Payment gateway timeout after 30000ms
2026-01-25T10:16:01Z ERROR auth Failed login attempt for user root from 10.0.0.55
2026-01-25T10:16:15Z WARN db Slow query detected: 2500ms for SELECT * FROM orders
2026-01-25T10:16:30Z ERROR payment Payment gateway timeout after 30000ms
2026-01-25T10:16:45Z ERROR auth Failed login attempt for user admin from 192.168.1.100
2026-01-25T10:17:00Z CRITICAL db Database master unreachable at 10.0.0.1:5432
2026-01-25T10:17:15Z ERROR api Connection timeout to upstream service after 5000ms
2026-01-25T10:17:30Z WARN cache Cache hit ratio low: 23% over last 300 seconds
2026-01-25T10:17:45Z ERROR api Connection timeout to upstream service after 5000ms
2026-01-25T10:18:00Z WARN db Slow query detected: 3100ms for SELECT * FROM users
2026-01-25T10:18:15Z ERROR auth Failed login attempt for user guest from 172.16.0.8
2026-01-25T10:18:30Z CRITICAL payment Payment database connection lost: host=db-02
2026-01-25T10:18:45Z ERROR api Connection timeout to upstream service after 8000ms
2026-01-25T10:19:00Z INFO auth User session expired for token 3f2504e0-4f89-11d3-9a0c-0305e82c3301
2026-01-25T10:19:15Z ERROR db Null pointer exception in query executor at line 442
2026-01-25T10:19:30Z WARN queue Job queue near capacity: 8750 of 10000 slots used
```

- [ ] **Step 2: Create sample_infrastructure.log**

```
2026-01-25T11:00:00Z ERROR registry Service registry unreachable at http://registry:8500
2026-01-25T11:00:15Z ERROR registry Service registry unreachable at http://registry:8500
2026-01-25T11:00:30Z WARN k8s Pod rollout stalled: payments-v2 in region us-east-1
2026-01-25T11:00:45Z ERROR registry Service registry unreachable at http://registry:8500
2026-01-25T11:01:00Z WARN k8s Pod rollout stalled: payments-v2 in region eu-west-1
2026-01-25T11:01:15Z ERROR tls SSL certificate expiring in 7 days for api.example.com
2026-01-25T11:01:30Z ERROR tls SSL certificate expiring in 5 days for auth.example.com
2026-01-25T11:01:45Z CRITICAL monitoring Monitoring system offline since 2026-01-25T10:55:00Z
2026-01-25T11:02:00Z ERROR backup Backup storage access denied for bucket s3://backups-prod
2026-01-25T11:02:15Z ERROR backup Backup storage access denied for bucket s3://backups-dr
2026-01-25T11:02:30Z WARN security Port scan detected from 203.0.113.42 on ports 22,80,443
```

- [ ] **Step 3: Run integration test — text output**

```bash
cd /Users/khusdeepsingh/docs/Learning/C++/LogTriage++ && ./build/logtriage --file sample_app.log --issue-level warn --min-count 2 --top 5 --samples
```

Expected: Text report showing clusters for auth failures, payment timeouts, api timeouts, db slow queries.

- [ ] **Step 4: Run integration test — JSON output**

```bash
./build/logtriage --file sample_app.log --file sample_infrastructure.log --output json --min-count 2
```

Expected: Valid JSON with `generated_at` and `clusters` array.

- [ ] **Step 5: Run integration test — stdin**

```bash
cat sample_app.log | ./build/logtriage --issue-level warn --min-count 2
```

Expected: Same text output as Step 3 (without --samples).

- [ ] **Step 6: Commit sample files**

```bash
git add sample_app.log sample_infrastructure.log
git commit -m "chore: add sample log files for integration testing"
```

---

### Task 11: Git init, .gitignore, final clean-up commit

**Files:**
- Create: `.gitignore`

- [ ] **Step 1: Initialize git repo (if not already done)**

```bash
cd /Users/khusdeepsingh/docs/Learning/C++/LogTriage++ && git init
```

- [ ] **Step 2: Create .gitignore**

```
# CMake build output
build/

# macOS metadata
.DS_Store

# Editor files
.vscode/
*.swp
```

- [ ] **Step 3: Stage everything and do final commit**

```bash
git add CMakeLists.txt src/ tests/ docs/ sample_app.log sample_infrastructure.log .gitignore
git commit -m "feat: initial LogTriage++ implementation

C++17 port of Python LogTriage tool.
- Parallel log parsing with std::thread (merge-after pattern)
- Regex-based message normalization (UUID, IP, hex, path, number masking)
- unordered_map clustering by component + normalized signature
- Text and JSON output formatters
- Full CLI parity with Python original (10+ flags)
- 4 Google Test unit tests covering normalize, parse, cluster, select
- Beginner-readable code with Python-comparison comments throughout"
```

- [ ] **Step 4: Verify final state**

```bash
./build/logtriage_tests && echo "All tests pass"
./build/logtriage --file sample_app.log --min-count 2 --verbose
```

Expected: 4 tests pass, text report with clusters printed.
