#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

class BrainfuckRecursion : public Benchmark {
private:
  struct OpInc {};
  struct OpDec {};
  struct OpAdvance {};
  struct OpDevance {};
  struct OpPrint {};
  struct OpLoop;

  using Op = std::variant<OpInc, OpDec, OpAdvance, OpDevance, OpPrint, OpLoop>;

  struct OpLoop {
    std::vector<Op> ops;
  };

  class Tape {
  private:
    std::vector<uint8_t> tape;
    size_t pos = 0;

  public:
    Tape() : tape(30000, 0) {}

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
      if (pos > 0) {
        pos--;
      }
    }
  };

  class Program {
  private:
    std::vector<Op> ops;

    std::vector<Op> parse(std::string::const_iterator &it,
                          const std::string::const_iterator &end);

    struct Visitor {
      Tape &tape;
      int64_t &result;

      void operator()(const OpInc &) const { tape.inc(); }
      void operator()(const OpDec &) const { tape.dec(); }
      void operator()(const OpAdvance &) const { tape.advance(); }
      void operator()(const OpDevance &) const { tape.devance(); }
      void operator()(const OpPrint &) const {
        result = (result << 2) + tape.get();
      }
      void operator()(const OpLoop &loop) const {
        while (tape.get() != 0) {
          for (const auto &op : loop.ops) {
            std::visit(*this, op);
          }
        }
      }
    };

  public:
    explicit Program(const std::string &code);
    int64_t run();
  };

  std::string text;
  uint32_t result_val;

public:
  BrainfuckRecursion();

  std::string name() const override;
  void warmup() override;
  void run(int) override;
  uint32_t checksum() override;
};