#include "benchmark.hpp"
#include "helper.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <re2/re2.h>
#include <thread>
#include <unordered_map>

#include "base64.hpp"
#include "binarytrees.hpp"
#include "brainfuck_array.hpp"
#include "brainfuck_recursion.hpp"
#include "buffer_hash.hpp"
#include "cache_simulation.hpp"
#include "calculator.hpp"
#include "compress.hpp"
#include "csv_parse.hpp"
#include "distance.hpp"
#include "fannkuchredux.hpp"
#include "game_of_life.hpp"
#include "graph_path.hpp"
#include "json_bench.hpp"
#include "log_parser.hpp"
#include "mandelbrot.hpp"
#include "matmul.hpp"
#include "maze.hpp"
#include "nbody.hpp"
#include "neural_net.hpp"
#include "sieve.hpp"
#include "sort.hpp"
#include "spectralnorm.hpp"
#include "template.hpp"
#include "text_raytracer.hpp"
#include "words.hpp"

int64_t Benchmark::warmup_iterations() {
  if (config_has(name())) {
    int64_t warmup = config_i64(name(), "warmup_iterations");
    if (warmup > 0)
      return warmup;
  }

  int64_t iters = iterations();
  return std::max<int64_t>(static_cast<int64_t>(iters * 0.2), 1LL);
}

void Benchmark::warmup() {
  int64_t prepare_iters = warmup_iterations();
  for (int64_t i = 0; i < prepare_iters; i++) {
    this->run(i);
  }
}

void Benchmark::run_all() {
  int64_t iters = iterations();
  for (int64_t i = 0; i < iters; i++) {
    this->run(i);
  }
}

void Benchmark::all(const std::string &single_bench,
                    const std::string &config_file) {
  load_config(config_file);

  double summary_time = 0.0;
  int ok = 0;
  int fails = 0;

  std::unordered_map<std::string, std::function<std::unique_ptr<Benchmark>()>>
      available_benches = {
          {"Binarytrees::Obj",
           []() { return std::make_unique<BinarytreesObj>(); }},
          {"Binarytrees::Arena",
           []() { return std::make_unique<BinarytreesArena>(); }},
          {"Brainfuck::Array",
           []() { return std::make_unique<BrainfuckArray>(); }},
          {"Brainfuck::Recursion",
           []() { return std::make_unique<BrainfuckRecursion>(); }},
          {"CLBG::Fannkuchredux",
           []() { return std::make_unique<Fannkuchredux>(); }},
          {"CLBG::Mandelbrot", []() { return std::make_unique<Mandelbrot>(); }},
          {"Matmul::Single", []() { return std::make_unique<Matmul1T>(); }},
          {"Matmul::T4", []() { return std::make_unique<Matmul4T>(); }},
          {"Matmul::T8", []() { return std::make_unique<Matmul8T>(); }},
          {"Matmul::T16", []() { return std::make_unique<Matmul16T>(); }},
          {"CLBG::Nbody", []() { return std::make_unique<Nbody>(); }},
          {"CLBG::Spectralnorm",
           []() { return std::make_unique<Spectralnorm>(); }},
          {"Base64::Encode", []() { return std::make_unique<Base64Encode>(); }},
          {"Base64::Decode", []() { return std::make_unique<Base64Decode>(); }},
          {"Json::Generate", []() { return std::make_unique<JsonGenerate>(); }},
          {"Json::ParseDom", []() { return std::make_unique<JsonParseDom>(); }},
          {"Json::ParseMapping",
           []() { return std::make_unique<JsonParseMapping>(); }},
          {"Etc::Sieve", []() { return std::make_unique<Sieve>(); }},
          {"Etc::TextRaytracer",
           []() { return std::make_unique<TextRaytracer>(); }},
          {"Etc::NeuralNet", []() { return std::make_unique<NeuralNet>(); }},
          {"Etc::Words", []() { return std::make_unique<Words>(); }},
          {"Sort::Quick", []() { return std::make_unique<SortQuick>(); }},
          {"Sort::Merge", []() { return std::make_unique<SortMerge>(); }},
          {"Sort::Self", []() { return std::make_unique<SortSelf>(); }},
          {"Graph::BFS", []() { return std::make_unique<GraphPathBFS>(); }},
          {"Graph::DFS", []() { return std::make_unique<GraphPathDFS>(); }},
          {"Graph::AStar", []() { return std::make_unique<GraphPathAStar>(); }},
          {"Hash::SHA256",
           []() { return std::make_unique<BufferHashSHA256>(); }},
          {"Hash::CRC32", []() { return std::make_unique<BufferHashCRC32>(); }},
          {"Etc::CacheSimulation",
           []() { return std::make_unique<CacheSimulation>(); }},
          {"Calculator::Ast",
           []() { return std::make_unique<CalculatorAst>(); }},
          {"Calculator::Interpreter",
           []() { return std::make_unique<CalculatorInterpreter>(); }},
          {"Etc::GameOfLife", []() { return std::make_unique<GameOfLife>(); }},
          {"Maze::Generator",
           []() { return std::make_unique<MazeGenerator>(); }},
          {"Maze::BFS", []() { return std::make_unique<MazeBFS>(); }},
          {"Maze::AStar", []() { return std::make_unique<MazeAStar>(); }},
          {"Compress::BWTEncode",
           []() { return std::make_unique<BWTEncode>(); }},
          {"Compress::BWTDecode",
           []() { return std::make_unique<BWTDecode>(); }},
          {"Compress::HuffEncode",
           []() { return std::make_unique<HuffEncode>(); }},
          {"Compress::HuffDecode",
           []() { return std::make_unique<HuffDecode>(); }},
          {"Compress::ArithEncode",
           []() { return std::make_unique<ArithEncode>(); }},
          {"Compress::ArithDecode",
           []() { return std::make_unique<ArithDecode>(); }},
          {"Compress::LZWEncode",
           []() { return std::make_unique<LZWEncode>(); }},
          {"Compress::LZWDecode",
           []() { return std::make_unique<LZWDecode>(); }},
          {"Distance::Jaro", []() { return std::make_unique<Jaro>(); }},
          {"Distance::NGram", []() { return std::make_unique<NGram>(); }},
          {"Etc::LogParser", []() { return std::make_unique<LogParser>(); }},
          {"Template::Regex",
           []() { return std::make_unique<TemplateRegex>(); }},
          {"Template::Parse",
           []() { return std::make_unique<TemplateParse>(); }},
          {"CSV::Parse", []() { return std::make_unique<CsvParse>(); }},
      };

  for (const auto &bench_name : config_keys()) {
    if (!single_bench.empty() &&
        to_lower(bench_name).find(to_lower(single_bench)) ==
            std::string::npos) {
      continue;
    }

    auto it = available_benches.find(bench_name);
    if (it != available_benches.end()) {
      std::cout << bench_name << ": ";
      std::cout.flush();

      auto bench = it->second();
      Helper::reset();
      bench->prepare();
      bench->warmup();
      Helper::reset();

      auto start = std::chrono::steady_clock::now();
      bench->run_all();
      auto end = std::chrono::steady_clock::now();

      std::chrono::duration<double> duration = end - start;

      uint32_t check = bench->checksum();
      uint32_t expect = static_cast<uint32_t>(bench->expected_checksum());
      if (check == expect) {
        std::cout << "OK ";
        ok++;
      } else {
        std::cout << "ERR[actual=" << check << ", expected=" << expect << "] ";
        fails++;
      }

      std::cout << "in " << std::fixed << std::setprecision(3)
                << duration.count() << "s" << std::endl;

      summary_time += duration.count();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } else {
      std::cout << "Warning: Benchmark '" << bench_name
                << "' defined in config but not found in code" << std::endl;
    }
  }

  if (ok + fails > 0) {
    std::cout << "Summary: " << std::fixed << std::setprecision(4)
              << summary_time << "s, " << (ok + fails) << ", " << ok << ", "
              << fails << std::endl;
  }

  if (fails > 0) {
    std::exit(1);
  }
}