#include "json_bench.hpp"
#include "simdjson.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>

JsonGenerate::JsonGenerate() : result(0), n(config_val("coords")) {
  data.reserve(static_cast<size_t>(n));
}

std::string JsonGenerate::name() const { return "Json::Generate"; }

void JsonGenerate::prepare() {
  for (int64_t i = 0; i < n; i++) {
    double x = custom_round(Helper::next_float(), 8);
    double y = custom_round(Helper::next_float(), 8);
    double z = custom_round(Helper::next_float(), 8);

    std::ostringstream name;
    name << std::fixed << std::setprecision(7) << Helper::next_float() << " "
         << Helper::next_int(10000);

    std::unordered_map<std::string, std::pair<int, bool>> opts = {
        {"1", {1, true}}};

    data.emplace_back(x, y, z, name.str(), opts);
  }
}

void JsonGenerate::run(int iteration_id) {
  (void)iteration_id;
  simdjson::builder::string_builder sb;

  sb.start_object();
  sb.escape_and_append_with_quotes("coordinates");
  sb.append_colon();
  sb.start_array();

  for (size_t i = 0; i < data.size(); ++i) {
    const auto &coord = data[i];

    sb.start_object();

    sb.append_key_value("x", coord.x);
    sb.append_comma();
    sb.append_key_value("y", coord.y);
    sb.append_comma();
    sb.append_key_value("z", coord.z);
    sb.append_comma();
    sb.append_key_value("name", coord.name);
    sb.append_comma();

    sb.escape_and_append_with_quotes("opts");
    sb.append_colon();
    sb.start_object();
    for (const auto &[key, value] : coord.opts) {
      sb.escape_and_append_with_quotes(key);
      sb.append_colon();
      sb.start_array();
      sb.append(value.first);
      sb.append_comma();
      sb.append(value.second);
      sb.end_array();
    }
    sb.end_object();

    sb.end_object();

    if (i < data.size() - 1) {
      sb.append_comma();
    }
  }

  sb.end_array();

  sb.append_comma();
  sb.append_key_value("info", "some info");

  sb.end_object();

  auto view = sb.view();
  if (view.error()) {
    throw std::runtime_error("JSON generation failed");
  }
  _result = std::string(view.value_unsafe());

  if (_result.size() >= 15 &&
      _result.compare(0, 15, "{\"coordinates\":") == 0) {
    result++;
  }
}

uint32_t JsonGenerate::checksum() { return result; }

JsonParseDom::JsonParseDom() : result_val(0) {}

std::string JsonParseDom::name() const { return "Json::ParseDom"; }

void JsonParseDom::prepare() {
  JsonGenerate jg;
  jg.n = config_val("coords");
  jg.prepare();
  jg.run(0);
  text = jg.get_result();
}

void JsonParseDom::run(int iteration_id) {
  (void)iteration_id;
  auto padded = simdjson::padded_string(text);
  simdjson::dom::parser parser;
  simdjson::dom::element doc = parser.parse(padded);

  double x_sum = 0.0, y_sum = 0.0, z_sum = 0.0;
  size_t len = 0;

  for (auto coord : doc["coordinates"]) {
    Coordinate c{coord["x"], coord["y"], coord["z"]};
    x_sum += c.x;
    y_sum += c.y;
    z_sum += c.z;
    len++;
  }

  double x = x_sum / len;
  double y = y_sum / len;
  double z = z_sum / len;

  result_val += Helper::checksum_f64(x) + Helper::checksum_f64(y) +
                Helper::checksum_f64(z);
}

uint32_t JsonParseDom::checksum() { return result_val; }

JsonParseMapping::JsonParseMapping() : result_val(0) {}

std::string JsonParseMapping::name() const { return "Json::ParseMapping"; }

void JsonParseMapping::prepare() {
  JsonGenerate jg;
  jg.n = config_val("coords");
  jg.prepare();
  jg.run(0);
  text = jg.get_result();
}

void JsonParseMapping::run(int iteration_id) {
  (void)iteration_id;
  simdjson::ondemand::parser parser;
  auto padded = simdjson::padded_string(text);
  auto doc = parser.iterate(padded);

  double x_sum = 0.0, y_sum = 0.0, z_sum = 0.0;
  size_t len = 0;

  for (auto coord : doc["coordinates"]) {
    Coordinate c{coord["x"], coord["y"], coord["z"]};

    x_sum += c.x;
    y_sum += c.y;
    z_sum += c.z;
    len++;
  }

  Coordinate avg{x_sum / len, y_sum / len, z_sum / len};
  result_val += Helper::checksum_f64(avg.x) + Helper::checksum_f64(avg.y) +
                Helper::checksum_f64(avg.z);
}

uint32_t JsonParseMapping::checksum() { return result_val; }