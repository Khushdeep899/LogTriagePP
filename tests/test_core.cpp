#include <gtest/gtest.h>
#include "model.hpp"
#include "parser.hpp"
#include "cluster.hpp"

TEST(NormalizeTest, MasksUuidIpNum) {
    std::string msg = "conn from 10.0.0.1 id=3f2504e0-4f89-11d3-9a0c-0305e82c3301 took 123ms";
    std::string out = normalize_message(msg);

    EXPECT_EQ(out.find("10.0.0.1"),  std::string::npos);
    EXPECT_EQ(out.find("3f2504e0"),  std::string::npos);
    EXPECT_EQ(out.find("123"),        std::string::npos);
    EXPECT_NE(out.find("<IP>"),       std::string::npos);
    EXPECT_NE(out.find("<UUID>"),     std::string::npos);
    EXPECT_NE(out.find("<NUM>"),      std::string::npos);
}

TEST(ParseLineTest, ExtractsFields) {
    std::string line = "2026-01-25T12:34:56Z ERROR auth Login failed for user admin";
    auto result = parse_line(line);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->level,     "ERROR");
    EXPECT_EQ(result->component, "auth");
    EXPECT_NE(result->message.find("Login failed"), std::string::npos);
}

TEST(ClusterTest, GroupsIdenticalSignatures) {
    LogEvent e1;
    e1.level = "ERROR"; e1.component = "db";
    e1.message = "connection timeout"; e1.normalized = "connection timeout";

    LogEvent e2;
    e2.level = "ERROR"; e2.component = "db";
    e2.message = "connection timeout after 3000ms"; e2.normalized = "connection timeout";

    auto clusters = build_clusters({e1, e2}, 5);

    ASSERT_EQ(clusters.size(), 1u);
    EXPECT_EQ(clusters.begin()->second.count, 2);
}

TEST(SelectTopTest, FiltersBySeverity) {
    std::unordered_map<std::string, Cluster> clusters;

    Cluster info_c;
    info_c.count = 5;
    info_c.level_counts["INFO"] = 5;
    clusters["svc|info msg"] = info_c;

    Cluster err_c;
    err_c.count = 3;
    err_c.level_counts["ERROR"] = 3;
    clusters["svc|err msg"] = err_c;

    auto top = select_top_clusters(clusters, 10, 1, 40);

    ASSERT_EQ(top.size(), 1u);
    EXPECT_EQ(top[0].second.level_counts.at("ERROR"), 3);
}
