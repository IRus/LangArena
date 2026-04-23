#include "helper.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;
using json = nlohmann::json;

json CONFIG;
thread_local int64_t Helper::last = 42;

void load_config(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Cannot open config file: " << filename << std::endl;
    return;
  }

  try {
    auto json_array = json::array();
    file >> json_array;

    CONFIG = json::object();
    for (const auto &item : json_array) {
      std::string name = item["name"];
      CONFIG[name] = item;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error parsing JSON config: " << e.what() << std::endl;
    CONFIG = json::object();
  }
}

void Helper::reset() { last = 42; }

int32_t Helper::next_int(int32_t max) {
  last = (last * IA + IC) % IM;
  return static_cast<int32_t>((last * max) / IM);
}

int32_t Helper::next_int(int32_t from, int32_t to) {
  return next_int(to - from + 1) + from;
}

double Helper::next_float(double max) {
  last = (last * IA + IC) % IM;
  return max * static_cast<double>(last) / IM;
}

uint32_t Helper::checksum(const std::string &v) {
  uint32_t hash = 5381;
  for (char c : v) {
    hash = ((hash << 5) + hash) + static_cast<uint8_t>(c);
  }
  return hash;
}

uint32_t Helper::checksum(const std::vector<uint8_t> &v) {
  uint32_t hash = 5381;
  for (uint8_t byte : v) {
    hash = ((hash << 5) + hash) + byte;
  }
  return hash;
}

uint32_t Helper::checksum_f64(double v) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(7) << v;
  return Helper::checksum(oss.str());
}

int64_t Helper::config_i64(const std::string &class_name,
                           const std::string &field_name) {
  try {
    if (CONFIG.contains(class_name) &&
        CONFIG[class_name].contains(field_name)) {
      return CONFIG[class_name][field_name].get<int64_t>();
    } else {
      throw std::runtime_error("Config not found for " + class_name +
                               ", field: " + field_name);
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 0;
  }
}

std::string Helper::config_s(const std::string &class_name,
                             const std::string &field_name) {
  try {
    if (CONFIG.contains(class_name) &&
        CONFIG[class_name].contains(field_name)) {
      return CONFIG[class_name][field_name].get<std::string>();
    } else {
      throw std::runtime_error("Config not found for " + class_name +
                               ", field: " + field_name);
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return "";
  }
}

double custom_round(double value, int32_t precision) {
  if (std::isnan(value) || std::isinf(value)) {
    return value;
  }

  double factor = std::pow(10.0, precision);
  double scaled = value * factor;

  double fraction = scaled - std::floor(scaled);

  if (std::abs(fraction) < 0.5) {
    return std::floor(scaled) / factor;
  } else if (std::abs(fraction) > 0.5) {
    return std::ceil(scaled) / factor;
  } else {
    return (std::round(scaled / 2.0) * 2.0) / factor;
  }
}

std::string to_lower(const std::string &str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}