#include "base64.hpp"
#include <sstream>

extern "C" {
#include "libbase64.h"
}

Base64Encode::Base64Encode() : result_val(0) {
  int64_t n = config_val("size");
  str = std::string(static_cast<size_t>(n), 'a');
  str2 = base64_encode_simple(str);
}

std::string Base64Encode::name() const { return "Base64::Encode"; }

size_t Base64Encode::b64_encode(char *dst, const char *src, size_t src_size) {
  size_t encoded_size;
  base64_encode(src, src_size, dst, &encoded_size, 0);
  return encoded_size;
}

std::string Base64Encode::base64_encode_simple(const std::string &input) {
  size_t encoded_len = encode_size(input.size());
  std::string result;
  result.resize(encoded_len);
  size_t actual_len = 0;
  base64_encode(input.data(), input.size(), &result[0], &actual_len, 0);
  result.resize(actual_len);
  return result;
}

void Base64Encode::run(int iteration_id) {
  (void)iteration_id;
  str2 = base64_encode_simple(str);
  result_val += str2.size();
}

uint32_t Base64Encode::checksum() {
  std::ostringstream ss;
  ss << "encode " << (str.size() > 4 ? str.substr(0, 4) + "..." : str) << " to "
     << (str2.size() > 4 ? str2.substr(0, 4) + "..." : str2) << ": "
     << result_val;
  return Helper::checksum(ss.str());
}

Base64Decode::Base64Decode() : result_val(0) {
  int64_t n = config_val("size");
  std::string str = std::string(static_cast<size_t>(n), 'a');

  size_t encoded_size = encode_size(str.size());
  str2.resize(encoded_size);
  size_t actual_encoded = 0;
  base64_encode(str.data(), str.size(), &str2[0], &actual_encoded, 0);
  str2.resize(actual_encoded);

  str3 = base64_decode_simple(str2);
}

std::string Base64Decode::name() const { return "Base64::Decode"; }

size_t Base64Decode::b64_decode(char *dst, const char *src, size_t src_size) {
  size_t decoded_size;
  if (base64_decode(src, src_size, dst, &decoded_size, 0) != 1) {
    return 0;
  }
  return decoded_size;
}

std::string Base64Decode::base64_decode_simple(const std::string &input) {
  size_t decoded_size = decode_size(input.size());
  std::string result;
  result.resize(decoded_size);
  size_t actual_len = b64_decode(&result[0], input.data(), input.size());
  result.resize(actual_len);
  return result;
}

void Base64Decode::run(int iteration_id) {
  (void)iteration_id;
  str3 = base64_decode_simple(str2);
  result_val += str3.size();
}

uint32_t Base64Decode::checksum() {
  std::ostringstream ss;
  ss << "decode " << (str2.size() > 4 ? str2.substr(0, 4) + "..." : str2)
     << " to " << (str3.size() > 4 ? str3.substr(0, 4) + "..." : str3) << ": "
     << result_val;
  return Helper::checksum(ss.str());
}