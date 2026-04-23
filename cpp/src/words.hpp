#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <string>

class Words : public Benchmark {
private:
  int words;
  int word_len;
  std::string text;
  uint32_t checksum_val;

public:
  Words();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};