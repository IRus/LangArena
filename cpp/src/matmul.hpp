#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <vector>

class Matmul1T : public Benchmark {
protected:
  uint32_t result_val;
  std::vector<std::vector<double>> a, b;

  std::vector<std::vector<double>> matgen(int n);
  std::vector<std::vector<double>>
  matmul(int n, const std::vector<std::vector<double>> &a,
         const std::vector<std::vector<double>> &b);

public:
  Matmul1T();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class Matmul4T : public Matmul1T {
protected:
  virtual int get_num_threads() const { return 4; }

  std::vector<std::vector<double>>
  matmul_parallel(int n, const std::vector<std::vector<double>> &a,
                  const std::vector<std::vector<double>> &b);

public:
  Matmul4T() = default;

  std::string name() const override;
  void run(int) override;
};

class Matmul8T : public Matmul4T {
protected:
  int get_num_threads() const override { return 8; }

public:
  Matmul8T() = default;
  std::string name() const override;
};

class Matmul16T : public Matmul4T {
protected:
  int get_num_threads() const override { return 16; }

public:
  Matmul16T() = default;
  std::string name() const override;
};