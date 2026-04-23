#include "log_parser.hpp"
#include <cstdio>
#include <re2/re2.h>
#include <unordered_map>

void LogParser::generate_log_line(std::string &str, int i) {
  str += IPS[i % IPS.size()] + " - - [" + std::to_string(i % 31) +
         "/Oct/2023:" + std::to_string(i % 60) + ":55:36 +0000] \"" +
         METHODS[i % METHODS.size()] + " ";

  if (i % 3 == 0)
    str += "/login?email=" + USERS[i % USERS.size()] + std::to_string(i % 100) +
           "@" + DOMAINS[i % DOMAINS.size()] + "&password=secret" +
           std::to_string(i % 10000);
  else if (i % 5 == 0) {
    str += "/api/data?token=";
    for (int j = 0; j < (i % 3) + 1; j++)
      str += "abcdef123456";
  } else if (i % 7 == 0) {
    char hex[16];
    snprintf(hex, sizeof(hex), "%x", i * 12345);
    str += std::string("/user/profile?session_id=sess_") + hex;
  } else
    str += PATHS[i % PATHS.size()];

  str += " HTTP/1.1\" " + std::to_string(STATUSES[i % STATUSES.size()]) +
         " 2326 \"http://" + DOMAINS[i % DOMAINS.size()] + "\" \"" +
         AGENTS[i % AGENTS.size()] + "\"\n";
}

std::string LogParser::name() const { return "Etc::LogParser"; }

void LogParser::prepare() {
  lines_count = config_val("lines_count");
  std::string log_builder;
  for (int i = 0; i < lines_count; i++)
    generate_log_line(log_builder, i);
  log = std::move(log_builder);

  for (size_t i = 0; i < PATTERNS.size(); i++) {
    compiled_patterns.push_back(std::make_unique<re2::RE2>(PATTERNS[i]));
    pattern_names.push_back(NAMES[i]);
  }
}

void LogParser::run(int) {
  std::unordered_map<std::string, int> matches;

  for (size_t i = 0; i < compiled_patterns.size(); i++) {
    const auto &re = compiled_patterns[i];
    if (!re || !re->ok())
      continue;

    re2::StringPiece input(log);
    int count = 0;
    re2::StringPiece match;

    while (re->Match(input, 0, input.size(), re2::RE2::UNANCHORED, &match, 1)) {
      count++;
      input.remove_prefix(match.data() - input.data() + match.size());
    }
    matches[pattern_names[i]] = count;
  }

  uint32_t total = 0;
  for (const auto &[_, c] : matches)
    total += c;
  checksum_val += total;
}

uint32_t LogParser::checksum() { return checksum_val; }