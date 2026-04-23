#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <vector>

class SortBenchmark : public Benchmark {
protected:
  std::vector<int32_t> data;
  int64_t size_val;
  uint32_t result_val;

  SortBenchmark();

public:
  virtual std::vector<int32_t> test() = 0;

  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class SortQuick : public SortBenchmark {
private:
  void quick_sort(std::vector<int32_t> &arr, int low, int high);

public:
  SortQuick() = default;

  std::string name() const override;
  std::vector<int32_t> test() override;
};

class SortMerge : public SortBenchmark {
private:
  void merge_sort_inplace(std::vector<int32_t> &arr);
  void merge_sort_helper(std::vector<int32_t> &arr, std::vector<int32_t> &temp,
                         int left, int right);
  void merge(std::vector<int32_t> &arr, std::vector<int32_t> &temp, int left,
             int mid, int right);

public:
  SortMerge() = default;

  std::string name() const override;
  std::vector<int32_t> test() override;
};

class SortSelf : public SortBenchmark {
public:
  SortSelf() = default;

  std::string name() const override;
  std::vector<int32_t> test() override;
};