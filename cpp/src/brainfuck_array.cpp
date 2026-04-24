#include "brainfuck_array.hpp"

BrainfuckArray::Program::Program(const std::string &text) {
  for (char c : text) {
    if (std::string("[]<>+-,.").find(c) != std::string::npos) {
      commands.push_back(static_cast<uint8_t>(c));
    }
  }

  jumps.resize(commands.size(), 0);
  std::vector<size_t> stack;

  for (size_t i = 0; i < commands.size(); ++i) {
    uint8_t cmd = commands[i];
    if (cmd == '[') {
      stack.push_back(i);
    } else if (cmd == ']' && !stack.empty()) {
      size_t start = stack.back();
      stack.pop_back();
      jumps[start] = i;
      jumps[i] = start;
    }
  }
}

int64_t BrainfuckArray::Program::run() {
  int64_t result = 0;
  Tape tape;
  size_t pc = 0;

  while (pc < commands.size()) {
    uint8_t cmd = commands[pc];
    switch (cmd) {
    case '+':
      tape.inc();
      break;
    case '-':
      tape.dec();
      break;
    case '>':
      tape.advance();
      break;
    case '<':
      tape.devance();
      break;
    case '[':
      if (tape.get() == 0) {
        pc = jumps[pc];
        continue;
      }
      break;
    case ']':
      if (tape.get() != 0) {
        pc = jumps[pc];
        continue;
      }
      break;
    case '.':
      result = (result << 2) + static_cast<int64_t>(tape.get());
      break;
    }
    pc++;
  }
  return result;
}

BrainfuckArray::BrainfuckArray() : result_val(0) {
  program_text = Helper::config_s(name(), "program");
  warmup_text = Helper::config_s(name(), "warmup_program");
}

std::string BrainfuckArray::name() const { return "Brainfuck::Array"; }

void BrainfuckArray::warmup() {
  int64_t prepare_iters = warmup_iterations();
  for (int64_t i = 0; i < prepare_iters; i++) {
    _run(warmup_text);
  }
}

void BrainfuckArray::run(int iteration_id) {
  (void)iteration_id;
  int64_t run_result = _run(program_text);
  result_val += static_cast<uint32_t>(run_result);
}

uint32_t BrainfuckArray::checksum() { return result_val; }

int64_t BrainfuckArray::_run(const std::string &text) {
  Program program(text);
  return program.run();
}