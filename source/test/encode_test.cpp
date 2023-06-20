#include "StarEncode.hpp"
#include "StarSha256.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(EncodeTest, Base64) {
  char const* data =
      "Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust "
      "of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, "
      "exceeds the short vehemence of any carnal pleasure.";
  ByteArray testSource = ByteArray(data, strlen(data));
  String testEncoded =
      "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhl"
      "ciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2YgdGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29u"
      "dGludWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55"
      "IGNhcm5hbCBwbGVhc3VyZS4=";

  String encoded = base64Encode(testSource);
  ByteArray decoded = base64Decode(encoded);

  EXPECT_EQ(encoded, testEncoded);
  EXPECT_EQ(decoded, testSource);
}

TEST(EncodeTest, SHA256) {
  char const* data =
      "Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust "
      "of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, "
      "exceeds the short vehemence of any carnal pleasure.";
  ByteArray testSource = ByteArray(data, strlen(data));
  ByteArray testHash = hexDecode("78fe75026c4390ceccc4e9e6a9428ba8ae5968b458e60b5eebecd682cf24bbf2");

  EXPECT_EQ(sha256(testSource), testHash);
}
