#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include "benchmark.hpp"
#include "helper.hpp"

int main(int argc, char *argv[]) {
  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count();
  std::cout << "start: " << now << std::endl;

  std::string config_file = "../run.js";
  if (argc > 1) {
    config_file = argv[1];
  }
  load_config(config_file);

  if (argc > 2) {
    Benchmark::all(argv[2], config_file);
  } else {
    Benchmark::all("", config_file);
  }

  std::ofstream file("/tmp/recompile_marker");
  if (file.is_open()) {
    file << "RECOMPILE_MARKER_0";
    file.close();
  }

  return 0;
}