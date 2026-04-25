#include "helper.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <simdjson.h>
#include <sstream>

static simdjson::dom::parser config_parser;
static simdjson::dom::document config_doc;
static simdjson::dom::element config_root;

thread_local int64_t Helper::last = 42;

void load_config(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Cannot open config file: " << filename << std::endl;
    return;
  }

  std::string json_str((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
  file.close();

  try {
    simdjson::padded_string padded = simdjson::padded_string(json_str);

    simdjson::dom::element doc;
    auto error = config_parser.parse_into_document(config_doc, padded).get(doc);
    if (error) {
      std::cerr << "Error parsing JSON config: " << error << std::endl;
      return;
    }

    config_root = doc;
  } catch (const simdjson::simdjson_error &e) {
    std::cerr << "Error parsing JSON config: " << e.what() << std::endl;
  }
}

bool config_has(const std::string &key) {
  if (config_root.type() == simdjson::dom::element_type::OBJECT) {
    simdjson::dom::object obj = config_root;
    auto result = obj[key];
    simdjson::dom::element elem;
    return !result.get(elem);
  } else if (config_root.type() == simdjson::dom::element_type::ARRAY) {
    for (auto item : simdjson::dom::array(config_root)) {
      std::string_view name = item["name"];
      if (name == key)
        return true;
    }
  }
  return false;
}

std::vector<std::string> config_keys() {
  std::vector<std::string> keys;

  if (config_root.type() == simdjson::dom::element_type::OBJECT) {
    for (auto [key, value] : simdjson::dom::object(config_root)) {
      keys.push_back(std::string(key));
    }
  } else if (config_root.type() == simdjson::dom::element_type::ARRAY) {
    for (auto item : simdjson::dom::array(config_root)) {
      std::string_view name = item["name"];
      keys.push_back(std::string(name));
    }
  }

  return keys;
}

int64_t Helper::config_i64(const std::string &class_name,
                           const std::string &field_name) {
  return ::config_i64(class_name, field_name);
}

std::string Helper::config_s(const std::string &class_name,
                             const std::string &field_name) {
  return ::config_s(class_name, field_name);
}

static simdjson::simdjson_result<simdjson::dom::element>
find_in_config(const std::string &key) {
  if (config_root.type() == simdjson::dom::element_type::OBJECT) {
    simdjson::dom::object obj = config_root;
    return obj[key];
  } else if (config_root.type() == simdjson::dom::element_type::ARRAY) {
    for (auto item : simdjson::dom::array(config_root)) {
      std::string_view name = item["name"];
      if (name == key) {
        return item;
      }
    }
  }
  return simdjson::NO_SUCH_FIELD;
}

int64_t config_i64(const std::string &key, const std::string &field) {
  try {
    auto result = find_in_config(key);
    simdjson::dom::element class_obj;
    auto error = result.get(class_obj);
    if (error) {

      return 0;
    }

    if (class_obj.type() != simdjson::dom::element_type::OBJECT) {
      return 0;
    }

    simdjson::dom::object obj = class_obj;
    simdjson::dom::element field_elem;
    error = obj[field].get(field_elem);
    if (error) {

      return 0;
    }

    switch (field_elem.type()) {
    case simdjson::dom::element_type::INT64:
      return int64_t(field_elem);
    case simdjson::dom::element_type::UINT64:
      return int64_t(uint64_t(field_elem));
    case simdjson::dom::element_type::DOUBLE:
      return int64_t(double(field_elem));
    case simdjson::dom::element_type::STRING: {
      std::string_view sv = field_elem;
      try {
        return std::stoll(std::string(sv));
      } catch (...) {
        return 0;
      }
    }
    default:
      return 0;
    }
  } catch (const std::exception &) {
    return 0;
  }
}

std::string config_s(const std::string &key, const std::string &field) {
  try {
    auto result = find_in_config(key);
    simdjson::dom::element class_obj;
    auto error = result.get(class_obj);
    if (error) {
      std::cerr << "Config not found for " << key << std::endl;
      return "";
    }

    simdjson::dom::object obj = class_obj;
    simdjson::dom::element field_elem;
    error = obj[field].get(field_elem);
    if (error) {
      std::cerr << "Field not found: " << field << std::endl;
      return "";
    }

    if (field_elem.type() == simdjson::dom::element_type::STRING) {
      return std::string(std::string_view(field_elem));
    }
    return "";
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return "";
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