#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <memory>
#include <vector>

class GraphPathBenchmark : public Benchmark {
protected:
  class Graph {
  public:
    int vertices;
    int jumps;
    int jump_len;
    std::vector<std::vector<int>> adj;

    Graph(int vertices, int jumps = 3, int jump_len = 100);
    void add_edge(int u, int v);
    void generate_random();
  };

  std::unique_ptr<Graph> graph;
  uint32_t result_val;

public:
  GraphPathBenchmark();

  void prepare() override;
  virtual int64_t test() = 0;
  void run(int) override;
  uint32_t checksum() override;
};

class GraphPathBFS : public GraphPathBenchmark {
private:
  int bfs_shortest_path(int start, int target);

public:
  GraphPathBFS() = default;

  std::string name() const override;
  int64_t test() override;
};

class GraphPathDFS : public GraphPathBenchmark {
private:
  int dfs_shortest_path(int start, int target);

public:
  GraphPathDFS() = default;

  std::string name() const override;
  int64_t test() override;
};

class GraphPathAStar : public GraphPathBenchmark {
private:
  struct Node {
    int vertex;
    int f_score;

    bool operator>(const Node &other) const { return f_score > other.f_score; }
  };

  int heuristic(int v, int target) const { return target - v; }
  int astar_shortest_path(int start, int target);

public:
  GraphPathAStar() = default;

  std::string name() const override;
  int64_t test() override;
};