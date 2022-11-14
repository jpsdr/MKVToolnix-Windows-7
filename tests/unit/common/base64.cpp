#include "common/common_pch.h"

#include "common/base64.h"

#include "tests/unit/init.h"

namespace {

std::string
decode(std::string const &src) {
  return mtx::base64::decode(src)->to_string();
}

std::string
encode(std::string const &src,
       bool line_breaks,
       int max_line_len) {
  return mtx::base64::encode(reinterpret_cast<unsigned char const *>(&src[0]), src.length(), line_breaks, max_line_len);
}

TEST(Base64, Encoding) {
  auto plain = "LABEL=ROOT / btrfs noatime,compress=lzo,user_subvol_rm_allowed 0 10"s;

  EXPECT_EQ("TEFCRUw9Uk9PVCAvIGJ0cmZzIG5vYXRpbWUsY29tcHJlc3M9bHpvLHVzZXJfc3Vidm9sX3JtX2FsbG93ZWQgMCAxMA=="s,               encode(plain, false, 0));
  EXPECT_EQ("TEFCRUw9Uk9PVCAvIGJ0cmZzIG5vYXRpbWUsY29tcHJlc3M9bHpvLHVzZXJfc3Vidm9sX3Jt\nX2FsbG93ZWQgMCAxMA=="s,             encode(plain, true,  72));
  EXPECT_EQ("TEFCRUw9Uk9P\nVCAvIGJ0cmZz\nIG5vYXRpbWUs\nY29tcHJlc3M9\nbHpvLHVzZXJf\nc3Vidm9sX3Jt\nX2FsbG93ZWQg\nMCAxMA=="s, encode(plain, true,  13));
}

TEST(Base64, Decoding) {
  auto plain = "LABEL=ROOT / btrfs noatime,compress=lzo,user_subvol_rm_allowed 0 10"s;

  EXPECT_EQ(plain, decode("TEFCRUw9Uk9PVCAvIGJ0cmZzIG5vYXRpbWUsY29tcHJlc3M9bHpvLHVzZXJfc3Vidm9sX3JtX2FsbG93ZWQgMCAxMA=="s));
  EXPECT_EQ(plain, decode("TEFCRUw9Uk9PVCAvIGJ0cmZzIG5vYXRpbWUsY29tcHJlc3M9bHpvLHVzZXJfc3Vidm9sX3Jt\nX2FsbG93ZWQgMCAxMA=="s));
  EXPECT_EQ(plain, decode("TEFCRUw9Uk9P\nVCAvIGJ0cmZz\nIG5vYXRpbWUs\nY29tcHJlc3M9\nbHpvLHVzZXJf\nc3Vidm9sX3Jt\nX2FsbG93ZWQg\nMCAxMA=="s));
}

}
