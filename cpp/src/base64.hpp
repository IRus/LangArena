#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <string>

class Base64Encode : public Benchmark {
private:
  std::string str;
  std::string str2;
  uint32_t result_val;

  static size_t encode_size(size_t size) {
    return (size_t)(size * 4 / 3.0) + 6;
  }

  static size_t b64_encode(char *dst, const char *src, size_t src_size);
  std::string base64_encode_simple(const std::string &input);

public:
  Base64Encode();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};

class Base64Decode : public Benchmark {
private:
  std::string str2;
  std::string str3;
  uint32_t result_val;

  static size_t decode_size(size_t size) {
    return (size_t)(size * 3 / 4.0) + 6;
  }

  static size_t encode_size(size_t size) {
    return (size_t)(size * 4 / 3.0) + 6;
  }

  static size_t b64_decode(char *dst, const char *src, size_t src_size);
  std::string base64_decode_simple(const std::string &input);

public:
  Base64Decode();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};