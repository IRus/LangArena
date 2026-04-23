#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <vector>

class GameOfLife : public Benchmark {
private:
  class Cell {
  private:
    bool alive;
    bool next_state;
    std::vector<Cell *> neighbors;

  public:
    Cell(bool alive = false);
    void add_neighbor(Cell *cell);
    void compute_next_state();
    void update();
    void set_alive(bool state);
    bool is_alive() const;
  };

  class Grid {
  private:
    int width;
    int height;
    std::vector<std::vector<Cell>> cells;

    void link_neighbors();

  public:
    Grid(int w, int h);

    void next_generation();
    std::vector<std::vector<Cell>> &get_cells();
    int count_alive() const;
    uint32_t compute_hash() const;
  };

  int32_t width;
  int32_t height;
  Grid grid;

public:
  GameOfLife();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};