#pragma once

#include "benchmark.hpp"
#include <cstdint>

class Sieve : public Benchmark {
private:
  int64_t limit;
  uint32_t checksum_val;

public:
  Sieve();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};