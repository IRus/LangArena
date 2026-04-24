#include "words.hpp"
#include <sstream>
#include <unordered_map>

Words::Words() : checksum_val(0) {
  words = config_val("words");
  word_len = config_val("word_len");
}

std::string Words::name() const { return "Etc::Words"; }

void Words::prepare() {
  const char chars[] = "abcdefghijklmnopqrstuvwxyz";
  const int char_count = 26;

  std::string result;
  result.reserve(words * (word_len + 1));

  for (int i = 0; i < words; ++i) {
    int len = Helper::next_int(word_len) + Helper::next_int(3) + 3;
    for (int j = 0; j < len; ++j) {
      result.push_back(chars[Helper::next_int(char_count)]);
    }
    if (i != words - 1) {
      result.push_back(' ');
    }
  }

  text = std::move(result);
}

void Words::run(int iteration_id) {
  (void)iteration_id;
  std::unordered_map<std::string, int> frequencies;
  std::istringstream iss(text);
  std::string word;

  while (iss >> word) {
    if (auto [it, inserted] = frequencies.try_emplace(word, 1); !inserted) {
      it->second++;
    }
  }

  std::string max_word;
  int max_count = 0;

  for (const auto &pair : frequencies) {
    if (pair.second > max_count) {
      max_count = pair.second;
      max_word = pair.first;
    }
  }

  checksum_val += static_cast<uint32_t>(max_count) +
                  Helper::checksum(max_word) +
                  static_cast<uint32_t>(frequencies.size());
}

uint32_t Words::checksum() { return checksum_val; }