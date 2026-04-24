#pragma once

#include "benchmark.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace re2 {
class RE2;
}

class LogParser : public Benchmark {
private:
  std::string log;
  uint32_t checksum_val{0};
  int lines_count{0};

  std::vector<std::unique_ptr<re2::RE2>> compiled_patterns;
  std::vector<std::string> pattern_names;

  const std::vector<std::string> IPS = [] {
    std::vector<std::string> ips;
    for (int i = 1; i <= 255; i++)
      ips.push_back("192.168.1." + std::to_string(i));
    return ips;
  }();

  const std::vector<std::string> METHODS = {"GET", "POST", "PUT", "DELETE"};
  const std::vector<std::string> PATHS = {"/index.html", "/api/users",
                                          "/admin",      "/images/logo.png",
                                          "/etc/passwd", "/wp-admin/setup.php"};
  const std::vector<int> STATUSES = {200, 201, 301, 302, 400, 401,
                                     403, 404, 500, 502, 503};
  const std::vector<std::string> AGENTS = {"Mozilla/5.0", "Googlebot/2.1",
                                           "curl/7.68.0", "scanner/2.0"};
  const std::vector<std::string> USERS = {"john", "jane", "alex",  "sarah",
                                          "mike", "anna", "david", "elena"};
  const std::vector<std::string> DOMAINS = {"example.com", "gmail.com",
                                            "yahoo.com",   "hotmail.com",
                                            "company.org", "mail.ru"};

  static constexpr std::array<const char *, 13> PATTERNS = {
      " [5][0-9]{2} | [4][0-9]{2} ",
      "(?i)bot|crawler|scanner|spider|indexing|crawl|robot|spider",
      "(?i)etc/passwd|wp-admin|\\.\\./",
      "\\d+\\.\\d+\\.\\d+\\.35",
      "/api/[^ \" ]+",
      "POST [^ ]* HTTP",
      "(?i)/login|/signin",
      "(?i)get|post|put",
      "[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}",
      "password=[^&\\s\"]+",
      "token=[^&\\s\"]+|api[_-]?key=[^&\\s\"]+",
      "session[_-]?id=[^&\\s\"]+",
      "\\[\\d+/\\w+/\\d+:1[3-7]:\\d+:\\d+ [+\\-]\\d+\\]"};

  static constexpr std::array<const char *, 13> NAMES = {
      "errors",        "bots",          "suspicious", "ips",    "api_calls",
      "post_requests", "auth_attempts", "methods",    "emails", "passwords",
      "tokens",        "sessions",      "peak_hours"};

  void generate_log_line(std::string &str, int i);

public:
  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};