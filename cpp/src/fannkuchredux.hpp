#pragma once

#include "benchmark.hpp"
#include <utility>

class Fannkuchredux : public Benchmark {
private:
  int64_t n;
  uint32_t result_val;

  std::pair<int, int> fannkuchredux(int n);

public:
  Fannkuchredux();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};