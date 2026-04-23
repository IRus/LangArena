#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <vector>

class BufferHashBenchmark : public Benchmark {
protected:
  std::vector<uint8_t> data;
  int64_t size_val;
  uint32_t result_val;

  BufferHashBenchmark();

public:
  virtual uint32_t test() = 0;

  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class BufferHashSHA256 : public BufferHashBenchmark {
private:
  struct SimpleSHA256 {
    static std::vector<uint8_t> digest(const std::vector<uint8_t> &data);
  };

public:
  BufferHashSHA256() = default;

  std::string name() const override;
  uint32_t test() override;
};

class BufferHashCRC32 : public BufferHashBenchmark {
private:
  uint32_t crc32(const std::vector<uint8_t> &data);

public:
  BufferHashCRC32() = default;

  std::string name() const override;
  uint32_t test() override;
};