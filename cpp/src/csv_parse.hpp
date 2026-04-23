#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <string>

class CsvParse : public Benchmark {
private:
  struct Point {
    double x, y, z;
    Point(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
  };

  int64_t rows;
  std::string data;
  uint32_t result_val;

public:
  CsvParse();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};