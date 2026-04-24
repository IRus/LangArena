#include "distance.hpp"
#include <algorithm>
#include <unordered_map>

std::vector<std::pair<std::string, std::string>>
generate_pair_strings(int64_t n, int64_t m) {
  std::vector<std::pair<std::string, std::string>> pairs;
  pairs.reserve(n);

  for (int64_t i = 0; i < n; ++i) {
    int len1 = Helper::next_int(m) + 4;
    int len2 = Helper::next_int(m) + 4;

    std::string str1, str2;
    str1.reserve(len1);
    str2.reserve(len2);

    for (int j = 0; j < len1; ++j)
      str1 += 'a' + Helper::next_int(10);
    for (int j = 0; j < len2; ++j)
      str2 += 'a' + Helper::next_int(10);

    pairs.emplace_back(std::move(str1), std::move(str2));
  }
  return pairs;
}

Jaro::Jaro()
    : count(config_val("count")), size(config_val("size")), result_val(0) {}

void Jaro::prepare() { pairs = generate_pair_strings(count, size); }

double Jaro::jaro(const std::string &s1, const std::string &s2) {
  size_t len1 = s1.size();
  size_t len2 = s2.size();

  if (len1 == 0 || len2 == 0)
    return 0.0;

  size_t match_dist = std::max(len1, len2) / 2;
  if (match_dist > 0)
    match_dist -= 1;

  std::vector<bool> s1_matches(len1, false);
  std::vector<bool> s2_matches(len2, false);

  int matches = 0;
  for (size_t i = 0; i < len1; ++i) {
    size_t start = i > match_dist ? i - match_dist : 0;
    size_t end = std::min<size_t>(len2 - 1, i + match_dist);
    for (size_t j = start; j <= end; ++j) {
      if (!s2_matches[j] && s1[i] == s2[j]) {
        s1_matches[i] = true;
        s2_matches[j] = true;
        matches++;
        break;
      }
    }
  }

  if (matches == 0)
    return 0.0;

  int transpositions = 0;
  size_t k = 0;
  for (size_t i = 0; i < len1; ++i) {
    if (s1_matches[i]) {
      while (k < len2 && !s2_matches[k])
        k++;
      if (k < len2) {
        if (s1[i] != s2[k])
          transpositions++;
        k++;
      }
    }
  }
  transpositions /= 2;

  double m = static_cast<double>(matches);
  volatile double jaro = (m / len1 + m / len2 + (m - transpositions) / m) / 3.0;
  return jaro;
}

void Jaro::run(int iteration_id) {
  (void)iteration_id;
  for (const auto &pair : pairs)
    result_val += static_cast<uint32_t>(jaro(pair.first, pair.second) * 1000);
}

uint32_t Jaro::checksum() { return result_val; }

std::string Jaro::name() const { return "Distance::Jaro"; }

NGram::NGram()
    : count(config_val("count")), size(config_val("size")), result_val(0) {}

void NGram::prepare() { pairs = generate_pair_strings(count, size); }

double NGram::ngram(const std::string &_s1, const std::string &_s2) {
  const auto &s1 = _s1;
  const auto &s2 = _s2;

  std::unordered_map<uint32_t, int> grams1;
  grams1.reserve(s1.size());

  for (size_t i = 0; i <= s1.size() - 4; ++i) {
    uint32_t gram = (static_cast<uint8_t>(s1[i]) << 24) |
                    (static_cast<uint8_t>(s1[i + 1]) << 16) |
                    (static_cast<uint8_t>(s1[i + 2]) << 8) |
                    static_cast<uint8_t>(s1[i + 3]);
    auto [it, inserted] = grams1.try_emplace(gram, 0);
    it->second++;
  }

  std::unordered_map<uint32_t, int> grams2;
  grams2.reserve(s2.size());
  int intersection = 0;

  for (size_t i = 0; i <= s2.size() - 4; ++i) {
    uint32_t gram = (static_cast<uint8_t>(s2[i]) << 24) |
                    (static_cast<uint8_t>(s2[i + 1]) << 16) |
                    (static_cast<uint8_t>(s2[i + 2]) << 8) |
                    static_cast<uint8_t>(s2[i + 3]);
    auto [it2, inserted2] = grams2.try_emplace(gram, 0);
    it2->second++;

    auto it1 = grams1.find(gram);
    if (it1 != grams1.end() && it2->second <= it1->second)
      intersection++;
  }

  size_t total = grams1.size() + grams2.size();
  return total > 0 ? static_cast<double>(intersection) / total : 0.0;
}

void NGram::run(int iteration_id) {
  (void)iteration_id;
  for (const auto &pair : pairs)
    result_val += static_cast<uint32_t>(ngram(pair.first, pair.second) * 1000);
}

uint32_t NGram::checksum() { return result_val; }

std::string NGram::name() const { return "Distance::NGram"; }