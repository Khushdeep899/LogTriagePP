#include <gtest/gtest.h>
#include "model.hpp"
#include "parser.hpp"
#include "cluster.hpp"

// ── Test 1: normalize_message ────────────────────────────────────────────────
//
// normalize_message should replace variable parts (UUIDs, IPs, numbers)
// with fixed placeholders so that two messages differing only in those values
// produce the same normalized string — making them cluster together.

TEST(NormalizeTest, MasksUuidIpNum) {
    std::string msg = "conn from 10.0.0.1 id=3f2504e0-4f89-11d3-9a0c-0305e82c3301 took 123ms";
    std::string out = normalize_message(msg);

    // Original values must be gone
    // std::string::npos is the "not found" sentinel returned by .find()
    // EXPECT_EQ(x, npos) means "assert x was not found in the string"
    EXPECT_EQ(out.find("10.0.0.1"),  std::string::npos);
    EXPECT_EQ(out.find("3f2504e0"),  std::string::npos);
    EXPECT_EQ(out.find("123"),        std::string::npos);

    // Placeholders must be present
    // EXPECT_NE means "expect not equal" — i.e., the substring WAS found
    EXPECT_NE(out.find("<IP>"),   std::string::npos);
    EXPECT_NE(out.find("<UUID>"), std::string::npos);
    EXPECT_NE(out.find("<NUM>"),  std::string::npos);
}

// ── Test 2: parse_line ───────────────────────────────────────────────────────
//
// parse_line should extract structured fields from a log line.
// The result is wrapped in std::optional — we must check .has_value() before use.

TEST(ParseLineTest, ExtractsFields) {
    std::string line = "2026-01-25T12:34:56Z ERROR auth Login failed for user admin";
    auto result = parse_line(line);

    // ASSERT_TRUE stops the test immediately if false — use for preconditions
    // EXPECT_* continues even on failure — use for individual field assertions
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->level,     "ERROR");
    EXPECT_EQ(result->component, "auth");
    // result->message is sugar for result.value().message
    EXPECT_NE(result->message.find("Login failed"), std::string::npos);
}

// ── Test 3: build_clusters ───────────────────────────────────────────────────
//
// Two events with the same component and normalized message should land in
// one cluster with count=2.

TEST(ClusterTest, GroupsIdenticalSignatures) {
    // Build two events that differ in raw message but share the same normalized form
    LogEvent e1;
    e1.level = "ERROR"; e1.component = "db";
    e1.message    = "connection timeout";
    e1.normalized = "connection timeout";  // already clean — no numbers to replace

    LogEvent e2;
    e2.level = "ERROR"; e2.component = "db";
    e2.message    = "connection timeout after 3000ms";
    e2.normalized = "connection timeout";  // same normalized form as e1

    auto clusters = build_clusters({e1, e2}, 5);

    // 1u = unsigned int literal — avoids signed/unsigned comparison warning
    ASSERT_EQ(clusters.size(), 1u);
    EXPECT_EQ(clusters.begin()->second.count, 2);
}
