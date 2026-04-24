#include "template.hpp"
#include <re2/re2.h>

static const std::vector<std::string> FIRST_NAMES = {
    "John", "Jane", "Bob", "Alice", "Charlie", "Diana", "Sarah", "Mike"};
static const std::vector<std::string> LAST_NAMES = {
    "Smith",  "Johnson", "Brown",  "Taylor",
    "Wilson", "Davis",   "Miller", "Jones"};
static const std::vector<std::string> CITIES = {"New York", "Los Angeles",
                                                "Chicago",  "Houston",
                                                "Phoenix",  "San Francisco"};
static const std::string LOREM =
    "Lorem {ipsum} dolor {sit} amet, consectetur adipiscing elit. Sed do "
    "eiusmod tempor incididunt ut labore {et} dolore magna aliqua. ";

void generate_template(std::string &text,
                       std::unordered_map<std::string, std::string> &vars,
                       int count) {
  vars.clear();
  std::string builder;
  builder.reserve(count * 200);

  builder += "<html><body>";
  builder += "<h1>{{TITLE}}</h1>";
  vars["TITLE"] = "Template title";
  builder += "<p>";
  builder += LOREM;
  builder += "</p>";
  builder += "<table>";

  for (int i = 0; i < count; i++) {
    if (i % 3 == 0) {
      builder += "<!-- {comment} -->";
    }
    builder += "<tr>";
    builder += "<td>{{ FIRST_NAME" + std::to_string(i) + " }}</td>";
    builder += "<td>{{LAST_NAME" + std::to_string(i) + "}}</td>";
    builder += "<td>{{  CITY" + std::to_string(i) + "  }}</td>";

    vars["FIRST_NAME" + std::to_string(i)] =
        FIRST_NAMES[i % FIRST_NAMES.size()];
    vars["LAST_NAME" + std::to_string(i)] = LAST_NAMES[i % LAST_NAMES.size()];
    vars["CITY" + std::to_string(i)] = CITIES[i % CITIES.size()];

    builder += "<td>{balance: " + std::to_string(i % 100) + "}</td>";
    builder += "</tr>\n";
  }

  builder += "</table>";
  builder += "</body></html>";
  text = std::move(builder);
}

TemplateRegex::TemplateRegex() : checksum_val(0), count(0) {
  count = config_val("count");
  regex = std::make_unique<re2::RE2>(PATTERN);
}

std::string TemplateRegex::name() const { return "Template::Regex"; }

void TemplateRegex::prepare() { generate_template(text, vars, count); }

void TemplateRegex::run(int iteration_id) {
  (void)iteration_id;
  size_t len = text.size();
  std::string result;
  result.reserve(static_cast<size_t>(len * 1.5));

  re2::StringPiece input(text);
  size_t lastPos = 0;
  re2::StringPiece matches[2];

  while (
      regex->Match(input, 0, input.size(), re2::RE2::UNANCHORED, matches, 2)) {
    size_t match_start = matches[0].data() - text.data();

    if (match_start > lastPos) {
      result.append(text.data() + lastPos, match_start - lastPos);
    }

    std::string key(matches[1].data(), matches[1].size());
    key = trim(key);

    auto it = vars.find(key);
    if (it != vars.end()) {
      result += it->second;
    }

    lastPos = match_start + matches[0].size();
    input.remove_prefix(matches[0].data() - input.data() + matches[0].size());
  }

  if (lastPos < text.size()) {
    result.append(text.data() + lastPos, text.size() - lastPos);
  }

  rendered = std::move(result);
  checksum_val += static_cast<uint32_t>(rendered.size());
}

uint32_t TemplateRegex::checksum() {
  return checksum_val + Helper::checksum(rendered);
}

TemplateParse::TemplateParse() : checksum_val(0), count(0) {
  count = config_val("count");
}

std::string TemplateParse::name() const { return "Template::Parse"; }

void TemplateParse::prepare() { generate_template(text, vars, count); }

void TemplateParse::run(int iteration_id) {
  (void)iteration_id;
  size_t len = text.size();
  std::string result;
  result.reserve(static_cast<size_t>(len * 1.5));

  size_t i = 0;
  while (i < len) {
    if (i + 1 < len && text[i] == '{' && text[i + 1] == '{') {
      size_t j = i + 2;
      while (j + 1 < len) {
        if (text[j] == '}' && text[j + 1] == '}') {
          break;
        }
        j++;
      }

      if (j + 1 < len) {
        std::string key = text.substr(i + 2, j - i - 2);
        key = trim(key);

        auto it = vars.find(key);
        if (it != vars.end()) {
          result += it->second;
        }
        i = j + 2;
        continue;
      }
    }

    result += text[i];
    i++;
  }

  rendered = std::move(result);
  checksum_val += static_cast<uint32_t>(rendered.size());
}

uint32_t TemplateParse::checksum() {
  return checksum_val + Helper::checksum(rendered);
}