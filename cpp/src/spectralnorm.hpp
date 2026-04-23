#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <vector>

class Spectralnorm : public Benchmark {
private:
  int64_t size_val;
  std::vector<double> u;
  std::vector<double> v;

  double eval_A(int i, int j) {
    return 1.0 / ((i + j) * (i + j + 1.0) / 2.0 + i + 1.0);
  }

  std::vector<double> eval_A_times_u(const std::vector<double> &u);
  std::vector<double> eval_At_times_u(const std::vector<double> &u);
  std::vector<double> eval_AtA_times_u(const std::vector<double> &u);

public:
  Spectralnorm();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};