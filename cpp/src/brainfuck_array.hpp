#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <string>
#include <vector>

class BrainfuckArray : public Benchmark {
private:
  class Tape {
  private:
    std::vector<uint8_t> tape;
    size_t pos;

  public:
    Tape() : tape(30000, 0), pos(0) {}

    uint8_t get() const { return tape[pos]; }
    void inc() { tape[pos]++; }
    void dec() { tape[pos]--; }
    void advance() {
      pos++;
      if (pos >= tape.size()) {
        tape.push_back(0);
      }
    }
    void devance() {
      if (pos > 0)
        pos--;
    }
  };

  class Program {
  private:
    std::vector<uint8_t> commands;
    std::vector<size_t> jumps;

  public:
    Program(const std::string &text);
    int64_t run();
  };

  std::string program_text;
  std::string warmup_text;
  uint32_t result_val;

  int64_t _run(const std::string &text);

public:
  BrainfuckArray();

  std::string name() const override;
  void warmup() override;
  void run(int) override;
  uint32_t checksum() override;
};