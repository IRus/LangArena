#include "brainfuck_recursion.hpp"

BrainfuckRecursion::Program::Program(const std::string &code) {
  auto it = code.begin();
  ops = parse(it, code.end());
}

std::vector<BrainfuckRecursion::Op>
BrainfuckRecursion::Program::parse(std::string::const_iterator &it,
                                   const std::string::const_iterator &end) {
  std::vector<Op> res;
  res.reserve(128);

  while (it != end) {
    char c = *it++;
    switch (c) {
    case '+':
      res.emplace_back(OpInc{});
      break;
    case '-':
      res.emplace_back(OpDec{});
      break;
    case '>':
      res.emplace_back(OpAdvance{});
      break;
    case '<':
      res.emplace_back(OpDevance{});
      break;
    case '.':
      res.emplace_back(OpPrint{});
      break;
    case '[': {
      auto loop_ops = parse(it, end);
      res.emplace_back(OpLoop{std::move(loop_ops)});
      break;
    }
    case ']':
      return res;
    default:
      break;
    }
  }
  return res;
}

int64_t BrainfuckRecursion::Program::run() {
  Tape tape;
  int64_t result = 0;
  Visitor visitor{tape, result};

  for (const auto &op : ops) {
    std::visit(visitor, op);
  }
  return result;
}

BrainfuckRecursion::BrainfuckRecursion() : result_val(0) {
  text = Helper::config_s(name(), "program");
}

std::string BrainfuckRecursion::name() const { return "Brainfuck::Recursion"; }

void BrainfuckRecursion::warmup() {
  int64_t prepare_iters = warmup_iterations();
  std::string warmup_program = Helper::config_s(name(), "warmup_program");
  for (int64_t i = 0; i < prepare_iters; i++) {
    Program(warmup_program).run();
  }
}

void BrainfuckRecursion::run(int iteration_id) {
  (void)iteration_id;
  result_val += Program(text).run();
}

uint32_t BrainfuckRecursion::checksum() { return result_val; }