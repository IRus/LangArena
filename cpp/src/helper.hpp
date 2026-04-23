#pragma once

#include <cstdint>
#include <json.hpp>
#include <string>
#include <vector>

class Helper {
public:
  static void reset();
  static int32_t next_int(int32_t max);
  static int32_t next_int(int32_t from, int32_t to);
  static double next_float(double max = 1.0);
  static uint32_t checksum(const std::string &v);
  static uint32_t checksum(const std::vector<uint8_t> &v);
  static uint32_t checksum_f64(double v);
  static int64_t config_i64(const std::string &class_name,
                            const std::string &field_name);
  static std::string config_s(const std::string &class_name,
                              const std::string &field_name);

private:
  static const int64_t IM = 139968;
  static const int64_t IA = 3877;
  static const int64_t IC = 29573;

  static thread_local int64_t last;
};

using json = nlohmann::json;
extern json CONFIG;

void load_config(const std::string &filename);
double custom_round(double value, int32_t precision);
std::string to_lower(const std::string &str);