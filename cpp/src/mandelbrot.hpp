#pragma once

#include "benchmark.hpp"

class Mandelbrot : public Benchmark {
private:
  static constexpr int ITER = 50;
  static constexpr double LIMIT = 2.0;

  int64_t w, h;
  std::vector<uint8_t> result_bin;

public:
  Mandelbrot();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};