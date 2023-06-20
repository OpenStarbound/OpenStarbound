#ifndef STAR_ENCODE_HPP
#define STAR_ENCODE_HPP

#include "StarString.hpp"
#include "StarByteArray.hpp"

namespace Star {

size_t hexEncode(char const* data, size_t len, char* output, size_t outLen = NPos);
size_t hexDecode(char const* src, size_t len, char* output, size_t outLen = NPos);
size_t nibbleDecode(char const* src, size_t len, char* output, size_t outLen = NPos);

size_t base64Encode(char const* data, size_t len, char* output, size_t outLen = NPos);
size_t base64Decode(char const* src, size_t len, char* output, size_t outLen = NPos);

String hexEncode(char const* data, size_t len);
String base64Encode(char const* data, size_t len);

String hexEncode(ByteArray const& data);
ByteArray hexDecode(String const& encodedData);

String base64Encode(ByteArray const& data);
ByteArray base64Decode(String const& encodedData);

}

#endif
