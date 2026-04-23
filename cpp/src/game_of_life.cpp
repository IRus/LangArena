#include "game_of_life.hpp"

GameOfLife::Cell::Cell(bool alive) : alive(alive), next_state(false) {}

void GameOfLife::Cell::add_neighbor(Cell *cell) { neighbors.push_back(cell); }

void GameOfLife::Cell::compute_next_state() {
  int alive_neighbors = 0;
  for (Cell *neighbor : neighbors)
    if (neighbor->alive)
      alive_neighbors++;

  if (alive)
    next_state = (alive_neighbors == 2 || alive_neighbors == 3);
  else
    next_state = (alive_neighbors == 3);
}

void GameOfLife::Cell::update() { alive = next_state; }

void GameOfLife::Cell::set_alive(bool state) { alive = state; }

bool GameOfLife::Cell::is_alive() const { return alive; }

GameOfLife::Grid::Grid(int w, int h) : width(w), height(h) {
  cells.resize(height);
  for (int y = 0; y < height; ++y) {
    cells[y].reserve(width);
    for (int x = 0; x < width; ++x) {
      cells[y].emplace_back(false);
    }
  }
  link_neighbors();
}

void GameOfLife::Grid::link_neighbors() {
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      Cell &cell = cells[y][x];
      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          if (dx == 0 && dy == 0)
            continue;
          int ny = (y + dy + height) % height;
          int nx = (x + dx + width) % width;
          cell.add_neighbor(&cells[ny][nx]);
        }
      }
    }
  }
}

void GameOfLife::Grid::next_generation() {
  for (auto &row : cells)
    for (auto &cell : row)
      cell.compute_next_state();
  for (auto &row : cells)
    for (auto &cell : row)
      cell.update();
}

std::vector<std::vector<GameOfLife::Cell>> &GameOfLife::Grid::get_cells() {
  return cells;
}

int GameOfLife::Grid::count_alive() const {
  int count = 0;
  for (const auto &row : cells)
    for (const auto &cell : row)
      if (cell.is_alive())
        count++;
  return count;
}

uint32_t GameOfLife::Grid::compute_hash() const {
  constexpr uint32_t FNV_OFFSET_BASIS = 2166136261UL;
  constexpr uint32_t FNV_PRIME = 16777619UL;
  uint32_t hash = FNV_OFFSET_BASIS;
  for (const auto &row : cells)
    for (const auto &cell : row) {
      uint32_t alive = cell.is_alive() ? 1U : 0U;
      hash = (hash ^ alive) * FNV_PRIME;
    }
  return hash;
}

GameOfLife::GameOfLife()
    : width(static_cast<int32_t>(config_val("w"))),
      height(static_cast<int32_t>(config_val("h"))), grid(width, height) {}

std::string GameOfLife::name() const { return "Etc::GameOfLife"; }

void GameOfLife::prepare() {
  for (auto &row : grid.get_cells())
    for (auto &cell : row)
      if (Helper::next_float(1.0) < 0.1)
        cell.set_alive(true);
}

void GameOfLife::run(int iteration_id) {
  (void)iteration_id;
  grid.next_generation();
}

uint32_t GameOfLife::checksum() {
  uint32_t alive = static_cast<uint32_t>(grid.count_alive());
  return grid.compute_hash() + alive;
}