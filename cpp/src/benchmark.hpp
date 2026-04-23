#pragma once

#include "helper.hpp"
#include <cstdint>
#include <string>
#include <vector>

class Benchmark {
public:
  virtual ~Benchmark() = default;
  virtual void run(int) = 0;
  virtual uint32_t checksum() = 0;

  virtual void prepare() {}
  virtual std::string name() const = 0;

  int64_t warmup_iterations();
  virtual void warmup();
  void run_all();

  int64_t config_val(const std::string &field_name) const {
    return Helper::config_i64(this->name(), field_name);
  }
  int64_t iterations() const { return config_val("iterations"); }
  int64_t expected_checksum() const { return config_val("checksum"); }

  static void all(const std::string &single_bench,
                  const std::string &config_file);
};