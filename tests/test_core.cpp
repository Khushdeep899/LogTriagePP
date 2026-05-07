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
