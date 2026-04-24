#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class JsonGenerate : public Benchmark {
private:
  struct Coordinate {
    double x, y, z;
    std::string name;
    std::unordered_map<std::string, std::pair<int, bool>> opts;

    Coordinate(
        double x, double y, double z, const std::string &name,
        const std::unordered_map<std::string, std::pair<int, bool>> &opts)
        : x(x), y(y), z(z), name(name), opts(opts) {}
  };

  std::vector<Coordinate> data;
  std::string _result;
  uint32_t result;

public:
  int64_t n;

  JsonGenerate();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;

  const std::string &get_result() const { return _result; }
};

class JsonParseDom : public Benchmark {
private:
  struct Coordinate {
    double x, y, z;
  };

  std::string text;
  uint32_t result_val;

public:
  JsonParseDom();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class JsonParseMapping : public Benchmark {
private:
  struct Coordinate {
    double x, y, z;
  };

  std::string text;
  uint32_t result_val;

public:
  JsonParseMapping();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};