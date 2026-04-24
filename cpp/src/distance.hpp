#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

std::vector<std::pair<std::string, std::string>>
generate_pair_strings(int64_t n, int64_t m);

class Jaro : public Benchmark {
private:
  int64_t count;
  int64_t size;
  std::vector<std::pair<std::string, std::string>> pairs;
  uint32_t result_val;

  double jaro(const std::string &s1, const std::string &s2);

public:
  Jaro();

  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
  std::string name() const override;
};

class NGram : public Benchmark {
private:
  int64_t count;
  int64_t size;
  std::vector<std::pair<std::string, std::string>> pairs;
  uint32_t result_val;

  double ngram(const std::string &_s1, const std::string &_s2);

public:
  NGram();

  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
  std::string name() const override;
};