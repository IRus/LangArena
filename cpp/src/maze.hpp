#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <memory>
#include <vector>

class MazeGenerator : public Benchmark {
public:
  enum class CellKind : uint8_t {
    Wall = 0,
    Space,
    Start,
    Finish,
    Border,
    Path
  };

  class Cell {
  public:
    CellKind kind;
    std::vector<Cell *> neighbors;
    int x;
    int y;

    Cell(int x, int y);
    bool is_walkable() const;
    void reset();
    uint32_t value() const;
  };

  class Maze {
  private:
    int w;
    int h;
    std::vector<std::vector<Cell>> cells;
    Cell *start;
    Cell *finish;

  public:
    Maze(int width, int height);

    void update_neighbors();
    void reset();
    void dig(Cell *start_cell);
    void ensure_open_finish(Cell *start_cell);
    void generate();

    Cell *middle_cell();
    Cell *get_start();
    Cell *get_finish();
    Cell *get_cell(int x, int y);

    uint32_t checksum() const;
    void print_to_console() const;
  };

private:
  uint32_t result_val;
  int32_t width;
  int32_t height;
  std::unique_ptr<Maze> maze;

public:
  MazeGenerator();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class MazeBFS : public Benchmark {
private:
  uint32_t result_val;
  int32_t width;
  int32_t height;
  std::unique_ptr<MazeGenerator::Maze> maze;
  std::vector<MazeGenerator::Cell *> path;

  std::vector<MazeGenerator::Cell *> bfs(MazeGenerator::Cell *start,
                                         MazeGenerator::Cell *target);
  uint32_t mid_cell_checksum(const std::vector<MazeGenerator::Cell *> &p);

public:
  MazeBFS();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class MazeAStar : public Benchmark {
private:
  struct Node {
    int f_score;
    int idx;

    bool operator>(const Node &other) const;
  };

  uint32_t result_val;
  int32_t width;
  int32_t height;
  std::unique_ptr<MazeGenerator::Maze> maze;
  std::vector<MazeGenerator::Cell *> path;

  int heuristic(MazeGenerator::Cell *a, MazeGenerator::Cell *b);
  int idx(int y, int x) const;
  std::vector<MazeGenerator::Cell *> astar(MazeGenerator::Cell *start,
                                           MazeGenerator::Cell *target);
  uint32_t mid_cell_checksum(const std::vector<MazeGenerator::Cell *> &p);

public:
  MazeAStar();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};