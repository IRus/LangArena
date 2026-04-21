#include "benchmark.h"

Benchmark *BinarytreesObj_create(void);
Benchmark *BinarytreesArena_create(void);
Benchmark *BrainfuckArray_create(void);
Benchmark *BrainfuckRecursion_create(void);
Benchmark *Fannkuchredux_create(void);
Benchmark *Mandelbrot_create(void);
Benchmark *Matmul_create(void);
Benchmark *Matmul4T_create(void);
Benchmark *Matmul8T_create(void);
Benchmark *Matmul16T_create(void);
Benchmark *Nbody_create(void);
Benchmark *Spectralnorm_create(void);
Benchmark *Base64Encode_create(void);
Benchmark *Base64Decode_create(void);
Benchmark *JsonGenerate_create(void);
Benchmark *JsonParseDom_create(void);
Benchmark *JsonParseMapping_create(void);
Benchmark *Sieve_create(void);
Benchmark *TextRaytracer_create(void);
Benchmark *NeuralNet_create(void);
Benchmark *SortQuick_create(void);
Benchmark *SortMerge_create(void);
Benchmark *SortSelf_create(void);
Benchmark *GraphPathBFS_create(void);
Benchmark *GraphPathDFS_create(void);
Benchmark *GraphPathAStar_create(void);
Benchmark *BufferHashSHA256_create(void);
Benchmark *BufferHashCRC32_create(void);
Benchmark *CacheSimulation_create(void);
Benchmark *CalculatorAst_create(void);
Benchmark *CalculatorInterpreter_create(void);
Benchmark *GameOfLife_create(void);
Benchmark *MazeGenerator_create(void);
Benchmark *MazeBFS_create(void);
Benchmark *MazeAStar_create(void);
Benchmark *BWTEncode_create(void);
Benchmark *BWTDecode_create(void);
Benchmark *HuffEncode_create(void);
Benchmark *HuffDecode_create(void);
Benchmark *ArithEncode_create(void);
Benchmark *ArithDecode_create(void);
Benchmark *LZWEncode_create(void);
Benchmark *LZWDecode_create(void);
Benchmark *Jaro_create(void);
Benchmark *NGram_create(void);
Benchmark *Words_create(void);
Benchmark *LogParser_create(void);
Benchmark *TemplateRegex_create(void);
Benchmark *TemplateParse_create(void);
Benchmark *CsvParse_create(void);

void register_all_benchmarks(void) {
  Benchmark_register("Binarytrees::Obj", BinarytreesObj_create);
  Benchmark_register("Binarytrees::Arena", BinarytreesArena_create);
  Benchmark_register("Brainfuck::Array", BrainfuckArray_create);
  Benchmark_register("Brainfuck::Recursion", BrainfuckRecursion_create);
  Benchmark_register("CLBG::Fannkuchredux", Fannkuchredux_create);
  Benchmark_register("CLBG::Mandelbrot", Mandelbrot_create);
  Benchmark_register("Matmul::Single", Matmul_create);
  Benchmark_register("Matmul::T4", Matmul4T_create);
  Benchmark_register("Matmul::T8", Matmul8T_create);
  Benchmark_register("Matmul::T16", Matmul16T_create);
  Benchmark_register("CLBG::Nbody", Nbody_create);
  Benchmark_register("CLBG::Spectralnorm", Spectralnorm_create);
  Benchmark_register("Base64::Encode", Base64Encode_create);
  Benchmark_register("Base64::Decode", Base64Decode_create);
  Benchmark_register("Json::Generate", JsonGenerate_create);
  Benchmark_register("Json::ParseDom", JsonParseDom_create);
  Benchmark_register("Json::ParseMapping", JsonParseMapping_create);
  Benchmark_register("Etc::Sieve", Sieve_create);
  Benchmark_register("Etc::TextRaytracer", TextRaytracer_create);
  Benchmark_register("Etc::NeuralNet", NeuralNet_create);
  Benchmark_register("Sort::Quick", SortQuick_create);
  Benchmark_register("Sort::Merge", SortMerge_create);
  Benchmark_register("Sort::Self", SortSelf_create);
  Benchmark_register("Graph::BFS", GraphPathBFS_create);
  Benchmark_register("Graph::DFS", GraphPathDFS_create);
  Benchmark_register("Graph::AStar", GraphPathAStar_create);
  Benchmark_register("Hash::SHA256", BufferHashSHA256_create);
  Benchmark_register("Hash::CRC32", BufferHashCRC32_create);
  Benchmark_register("Etc::CacheSimulation", CacheSimulation_create);
  Benchmark_register("Calculator::Ast", CalculatorAst_create);
  Benchmark_register("Calculator::Interpreter", CalculatorInterpreter_create);
  Benchmark_register("Etc::GameOfLife", GameOfLife_create);
  Benchmark_register("Maze::Generator", MazeGenerator_create);
  Benchmark_register("Maze::BFS", MazeBFS_create);
  Benchmark_register("Maze::AStar", MazeAStar_create);
  Benchmark_register("Compress::BWTEncode", BWTEncode_create);
  Benchmark_register("Compress::BWTDecode", BWTDecode_create);
  Benchmark_register("Compress::HuffEncode", HuffEncode_create);
  Benchmark_register("Compress::HuffDecode", HuffDecode_create);
  Benchmark_register("Compress::ArithEncode", ArithEncode_create);
  Benchmark_register("Compress::ArithDecode", ArithDecode_create);
  Benchmark_register("Compress::LZWEncode", LZWEncode_create);
  Benchmark_register("Compress::LZWDecode", LZWDecode_create);
  Benchmark_register("Distance::Jaro", Jaro_create);
  Benchmark_register("Distance::NGram", NGram_create);
  Benchmark_register("Etc::Words", Words_create);
  Benchmark_register("Etc::LogParser", LogParser_create);
  Benchmark_register("Template::Regex", TemplateRegex_create);
  Benchmark_register("Template::Parse", TemplateParse_create);
  Benchmark_register("CSV::Parse", CsvParse_create);
}

int main(int argc, char *argv[]) {

  struct timespec start_time;
  clock_gettime(CLOCK_REALTIME, &start_time);
  printf("start: %ld\n",
         start_time.tv_sec * 1000 + start_time.tv_nsec / 1000000);

  const char *config_file = argc > 1 ? argv[1] : "../run.js";
  Helper_load_config(config_file);

  register_all_benchmarks();

  const char *single_bench = argc > 2 ? argv[2] : NULL;
  Benchmark_all(single_bench);

  Helper_free_config();

  if (benchmark_factories) {
    free(benchmark_factories);
    benchmark_factories = NULL;
    benchmark_factories_count = 0;
    benchmark_factories_capacity = 0;
  }

  FILE *f = fopen("/tmp/recompile_marker", "w");
  if (f) {
    fprintf(f, "RECOMPILE_MARKER_0");
    fclose(f);
  }

  return 0;
}
