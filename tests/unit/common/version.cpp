#include "common/common_pch.h"

#include "common/version.h"

#include "gtest/gtest.h"

namespace {

TEST(VersionNumberT, Parsing11) {
  version_number_t v{"11"};

  ASSERT_TRUE(v.valid);
  ASSERT_EQ(1,  v.parts.size());
  EXPECT_EQ(11, v.parts[0]);
  EXPECT_EQ(0,  v.build);
}

TEST(VersionNumberT, Parsing11_12) {
  version_number_t v{"11.12"};

  ASSERT_TRUE(v.valid);
  ASSERT_EQ(2,  v.parts.size());
  EXPECT_EQ(11, v.parts[0]);
  EXPECT_EQ(12, v.parts[1]);
}

TEST(VersionNumberT, Parsing1_2_3_4) {
  version_number_t v{"1.2.3.4"};

  ASSERT_TRUE(v.valid);
  ASSERT_EQ(4, v.parts.size());
  EXPECT_EQ(1, v.parts[0]);
  EXPECT_EQ(2, v.parts[1]);
  EXPECT_EQ(3, v.parts[2]);
  EXPECT_EQ(4, v.parts[3]);
  EXPECT_EQ(0, v.build);
}

TEST(VersionNumberT, Parsing1_2_3_4_build_9876) {
  version_number_t v{"1.2.3.4 build 9876"};

  ASSERT_TRUE(v.valid);
  ASSERT_EQ(4,    v.parts.size());
  EXPECT_EQ(1,    v.parts[0]);
  EXPECT_EQ(2,    v.parts[1]);
  EXPECT_EQ(3,    v.parts[2]);
  EXPECT_EQ(4,    v.parts[3]);
  EXPECT_EQ(9876, v.build);
}

TEST(VersionNumberT, Parsing11_build_456) {
  version_number_t v{"11 build 456"};

  ASSERT_TRUE(v.valid);
  ASSERT_EQ(1,   v.parts.size());
  EXPECT_EQ(11,  v.parts[0]);
  EXPECT_EQ(456, v.build);
}

TEST(VersionNumberT, Parsing_mkvmerge_v1_2_3) {
  version_number_t v{"mkvmerge v1.2.3"};

  ASSERT_TRUE(v.valid);
  ASSERT_EQ(3, v.parts.size());
  EXPECT_EQ(1, v.parts[0]);
  EXPECT_EQ(2, v.parts[1]);
  EXPECT_EQ(3, v.parts[2]);
  EXPECT_EQ(0, v.build);
}

TEST(VersionNumberT, Parsing_11_12_13_build20170422_123) {
  version_number_t v{"11.12.13-build20170422-123"};

  ASSERT_TRUE(v.valid);
  ASSERT_EQ(3,   v.parts.size());
  EXPECT_EQ(11,  v.parts[0]);
  EXPECT_EQ(12,  v.parts[1]);
  EXPECT_EQ(13,  v.parts[2]);
  EXPECT_EQ(123, v.build);
}

TEST(VersionNumberT, ParsingInvalid) {
  EXPECT_FALSE(version_number_t{}.valid);
  EXPECT_FALSE(version_number_t{""}.valid);
  EXPECT_FALSE(version_number_t{"11 12"}.valid);
  EXPECT_FALSE(version_number_t{"11 qwe 12"}.valid);
}

TEST(VersionNumberT, Compare) {
  EXPECT_EQ(0, version_number_t{"1.2.3"}         .compare(version_number_t{"1.2.3"}));
  EXPECT_EQ(0, version_number_t{"1.2.3.0"}       .compare(version_number_t{"1.2.3"}));
  EXPECT_EQ(0, version_number_t{"1.2.3"}         .compare(version_number_t{"1.2.3.0"}));
  EXPECT_EQ(0, version_number_t{"1.2.3"}         .compare(version_number_t{"1.2.3 build 0"}));
  EXPECT_EQ(0, version_number_t{"1.2.3.0"}       .compare(version_number_t{"1.2.3 build 0"}));
  EXPECT_EQ(0, version_number_t{"11"}            .compare(version_number_t{"11.0.0.0"}));

  EXPECT_EQ(1, version_number_t{"1.2.3"}         .compare(version_number_t{"1.0.0"}));
  EXPECT_EQ(1, version_number_t{"1.2.4"}         .compare(version_number_t{"1.2.3.9"}));
  EXPECT_EQ(1, version_number_t{"1.2.10"}        .compare(version_number_t{"1.2.9"}));
  EXPECT_EQ(1, version_number_t{"1.2.3 build 1"} .compare(version_number_t{"1.2.3"}));
  EXPECT_EQ(1, version_number_t{"1.2.3 build 10"}.compare(version_number_t{"1.2.3 build 9"}));
  EXPECT_EQ(1, version_number_t{"11"}            .compare(version_number_t{"9.9.9"}));

  EXPECT_EQ(-1, version_number_t{"1.0.0"}        .compare(version_number_t{"1.2.3"}));
  EXPECT_EQ(-1, version_number_t{"1.2.3.9"}      .compare(version_number_t{"1.2.4"}));
  EXPECT_EQ(-1, version_number_t{"1.2.9"}        .compare(version_number_t{"1.2.10"}));
  EXPECT_EQ(-1, version_number_t{"1.2.3"}        .compare(version_number_t{"1.2.3 build 1"}));
  EXPECT_EQ(-1, version_number_t{"1.2.3 build 9"}.compare(version_number_t{"1.2.3 build 10"}));
  EXPECT_EQ(-1, version_number_t{"9.9.9"}        .compare(version_number_t{"11"}));
}

TEST(VersionNumberT, LessThan) {
  EXPECT_FALSE(version_number_t{"1.2.3"}          < version_number_t{"1.2.3"});
  EXPECT_FALSE(version_number_t{"1.2.3.0"}        < version_number_t{"1.2.3"});
  EXPECT_FALSE(version_number_t{"1.2.3"}          < version_number_t{"1.2.3.0"});
  EXPECT_FALSE(version_number_t{"1.2.3"}          < version_number_t{"1.2.3 build 0"});
  EXPECT_FALSE(version_number_t{"1.2.3.0"}        < version_number_t{"1.2.3 build 0"});
  EXPECT_FALSE(version_number_t{"11"}             < version_number_t{"11.0.0.0"});

  EXPECT_FALSE(version_number_t{"1.2.3"}          < version_number_t{"1.0.0"});
  EXPECT_FALSE(version_number_t{"1.2.4"}          < version_number_t{"1.2.3.9"});
  EXPECT_FALSE(version_number_t{"1.2.10"}         < version_number_t{"1.2.9"});
  EXPECT_FALSE(version_number_t{"1.2.3 build 1"}  < version_number_t{"1.2.3"});
  EXPECT_FALSE(version_number_t{"1.2.3 build 10"} < version_number_t{"1.2.3 build 9"});
  EXPECT_FALSE(version_number_t{"11"}             < version_number_t{"9.9.9"});

  EXPECT_TRUE(version_number_t{"1.0.0"}           < version_number_t{"1.2.3"});
  EXPECT_TRUE(version_number_t{"1.2.3.9"}         < version_number_t{"1.2.4"});
  EXPECT_TRUE(version_number_t{"1.2.9"}           < version_number_t{"1.2.10"});
  EXPECT_TRUE(version_number_t{"1.2.3"}           < version_number_t{"1.2.3 build 1"});
  EXPECT_TRUE(version_number_t{"1.2.3 build 9"}   < version_number_t{"1.2.3 build 10"});
  EXPECT_TRUE(version_number_t{"9.9.9"}           < version_number_t{"11"});
}

TEST(VersionNumberT, Stringification) {
  EXPECT_EQ(std::string{"<invalid>"},       version_number_t{}                                   .to_string());
  EXPECT_EQ(std::string{"<invalid>"},       version_number_t{""}                                 .to_string());
  EXPECT_EQ(std::string{"1.2.3"},           version_number_t{"1.2.3"}                            .to_string());
  EXPECT_EQ(std::string{"1.2.3.0"},         version_number_t{"1.2.3.0"}                          .to_string());
  EXPECT_EQ(std::string{"1.2.3.0"},         version_number_t{"1.2.3.0 build 0"}                  .to_string());
  EXPECT_EQ(std::string{"1.2.3.4"},         version_number_t{"1.2.3.4"}                          .to_string());
  EXPECT_EQ(std::string{"1.2.3 build 5"},   version_number_t{"1.2.3 build 5"}                    .to_string());
  EXPECT_EQ(std::string{"1.2.3.0 build 5"}, version_number_t{"1.2.3.0 build 5"}                  .to_string());
  EXPECT_EQ(std::string{"1.2.3.0 build 5"}, version_number_t{"mkvmerge v1.2.3.0-build20170422-5"}.to_string());
  EXPECT_EQ(std::string{"11"},              version_number_t{"11"}                               .to_string());
  EXPECT_EQ(std::string{"11 build 1213"},   version_number_t{"11 build 1213"}                    .to_string());
}

}
