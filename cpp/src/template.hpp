#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace re2 {
class RE2;
}

void generate_template(std::string &text,
                       std::unordered_map<std::string, std::string> &vars,
                       int count);

class TemplateRegex : public Benchmark {
private:
  std::string text;
  std::string rendered;
  uint32_t checksum_val;
  int count;
  std::unordered_map<std::string, std::string> vars;
  std::unique_ptr<re2::RE2> regex;

  static constexpr const char *PATTERN = R"(\{\{(.*?)\}\})";

  std::string trim(const std::string &str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    if (start == std::string::npos)
      return "";
    return str.substr(start, end - start + 1);
  }

public:
  TemplateRegex();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class TemplateParse : public Benchmark {
private:
  std::string text;
  std::string rendered;
  uint32_t checksum_val;
  int count;
  std::unordered_map<std::string, std::string> vars;

  std::string trim(const std::string &str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    if (start == std::string::npos)
      return "";
    return str.substr(start, end - start + 1);
  }

public:
  TemplateParse();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};