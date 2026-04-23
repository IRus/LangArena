#include "maze.hpp"
#include "helper.hpp"
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <limits>
#include <queue>

MazeGenerator::Cell::Cell(int x, int y) : kind(CellKind::Wall), x(x), y(y) {
  neighbors.reserve(4);
}

bool MazeGenerator::Cell::is_walkable() const {
  return kind == CellKind::Space || kind == CellKind::Start ||
         kind == CellKind::Finish;
}

void MazeGenerator::Cell::reset() {
  if (kind == CellKind::Space)
    kind = CellKind::Wall;
}

uint32_t MazeGenerator::Cell::value() const {
  return static_cast<uint32_t>(kind);
}

MazeGenerator::Maze::Maze(int width, int height) : w(width), h(height) {
  cells.reserve(h);
  for (int y = 0; y < h; ++y) {
    auto &row = cells.emplace_back();
    row.reserve(w);
    for (int x = 0; x < w; ++x) {
      row.emplace_back(x, y);
    }
  }

  start = &cells[1][1];
  finish = &cells[h - 2][w - 2];
  start->kind = CellKind::Start;
  finish->kind = CellKind::Finish;

  update_neighbors();
}

void MazeGenerator::Maze::update_neighbors() {
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      auto &cell = cells[y][x];

      if (x > 0 && y > 0 && x < w - 1 && y < h - 1) {
        cell.neighbors = {&cells[y - 1][x], &cells[y + 1][x], &cells[y][x + 1],
                          &cells[y][x - 1]};

        for (int t = 0; t < 4; ++t) {
          int i = Helper::next_int(4);
          int j = Helper::next_int(4);
          if (i != j)
            std::swap(cell.neighbors[i], cell.neighbors[j]);
        }
      } else {
        cell.kind = CellKind::Border;
      }
    }
  }
}

void MazeGenerator::Maze::reset() {
  for (auto &row : cells)
    for (auto &cell : row)
      cell.reset();
  start->kind = CellKind::Start;
  finish->kind = CellKind::Finish;
}

void MazeGenerator::Maze::dig(Cell *start_cell) {
  if (!start_cell)
    return;
  std::vector<Cell *> stack;
  stack.push_back(start_cell);

  while (!stack.empty()) {
    auto *cell = stack.back();
    stack.pop_back();

    int walkable = 0;
    for (auto *n : cell->neighbors)
      if (n->is_walkable())
        ++walkable;
    if (walkable != 1)
      continue;

    cell->kind = CellKind::Space;
    for (auto *n : cell->neighbors)
      if (n->kind == CellKind::Wall)
        stack.push_back(n);
  }
}

void MazeGenerator::Maze::ensure_open_finish(Cell *start_cell) {
  if (!start_cell)
    return;
  std::vector<Cell *> stack;
  stack.push_back(start_cell);

  while (!stack.empty()) {
    auto *cell = stack.back();
    stack.pop_back();
    cell->kind = CellKind::Space;

    int walkable = 0;
    for (auto *n : cell->neighbors)
      if (n->is_walkable())
        ++walkable;
    if (walkable > 1)
      continue;

    for (auto *n : cell->neighbors)
      if (n->kind == CellKind::Wall)
        stack.push_back(n);
  }
}

void MazeGenerator::Maze::generate() {
  for (auto *n : start->neighbors)
    if (n->kind == CellKind::Wall)
      dig(n);
  for (auto *n : finish->neighbors)
    if (n->kind == CellKind::Wall)
      ensure_open_finish(n);
}

MazeGenerator::Cell *MazeGenerator::Maze::middle_cell() {
  return &cells[h / 2][w / 2];
}
MazeGenerator::Cell *MazeGenerator::Maze::get_start() { return start; }
MazeGenerator::Cell *MazeGenerator::Maze::get_finish() { return finish; }

MazeGenerator::Cell *MazeGenerator::Maze::get_cell(int x, int y) {
  if (x >= 0 && x < w && y >= 0 && y < h)
    return &cells[y][x];
  return nullptr;
}

uint32_t MazeGenerator::Maze::checksum() const {
  uint32_t hasher = 2166136261UL;
  uint32_t prime = 16777619UL;
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
      if (cells[y][x].kind == CellKind::Space) {
        uint32_t val = static_cast<uint32_t>(x * y);
        hasher = (hasher ^ val) * prime;
      }
  return hasher;
}

void MazeGenerator::Maze::print_to_console() const {
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      switch (cells[y][x].kind) {
      case CellKind::Space:
        std::cout << ' ';
        break;
      case CellKind::Wall:
        std::cout << "\033[34m#\033[0m";
        break;
      case CellKind::Border:
        std::cout << "\033[31mO\033[0m";
        break;
      case CellKind::Start:
        std::cout << "\033[32m>\033[0m";
        break;
      case CellKind::Finish:
        std::cout << "\033[32m<\033[0m";
        break;
      case CellKind::Path:
        std::cout << "\033[33m.\033[0m";
        break;
      }
    }
    std::cout << '\n';
  }
  std::cout << '\n';
}

MazeGenerator::MazeGenerator() : result_val(0) {
  width = static_cast<int32_t>(config_val("w"));
  height = static_cast<int32_t>(config_val("h"));
  maze = std::make_unique<Maze>(width, height);
}

std::string MazeGenerator::name() const { return "Maze::Generator"; }

void MazeGenerator::prepare() {}

void MazeGenerator::run(int) {
  maze->reset();
  maze->generate();
  result_val += maze->middle_cell()->value();
}

uint32_t MazeGenerator::checksum() { return result_val + maze->checksum(); }

MazeBFS::MazeBFS() : result_val(0) {
  width = static_cast<int32_t>(config_val("w"));
  height = static_cast<int32_t>(config_val("h"));
  maze = std::make_unique<MazeGenerator::Maze>(width, height);
}

std::string MazeBFS::name() const { return "Maze::BFS"; }

void MazeBFS::prepare() { maze->generate(); }

std::vector<MazeGenerator::Cell *> MazeBFS::bfs(MazeGenerator::Cell *start,
                                                MazeGenerator::Cell *target) {
  if (start == target)
    return {start};

  struct PathNode {
    MazeGenerator::Cell *cell;
    int parent;
  };
  std::deque<int> queue;
  std::vector<std::vector<bool>> visited(height,
                                         std::vector<bool>(width, false));
  std::vector<PathNode> path_nodes;

  visited[start->y][start->x] = true;
  path_nodes.push_back({start, -1});
  queue.push_back(0);

  while (!queue.empty()) {
    int path_id = queue.front();
    queue.pop_front();
    auto *cell = path_nodes[path_id].cell;

    for (auto *neighbor : cell->neighbors) {
      if (neighbor == target) {
        std::vector<MazeGenerator::Cell *> result = {target};
        int current = path_id;
        while (current >= 0) {
          result.push_back(path_nodes[current].cell);
          current = path_nodes[current].parent;
        }
        std::reverse(result.begin(), result.end());
        return result;
      }
      if (neighbor->is_walkable() && !visited[neighbor->y][neighbor->x]) {
        visited[neighbor->y][neighbor->x] = true;
        path_nodes.push_back({neighbor, path_id});
        queue.push_back(path_nodes.size() - 1);
      }
    }
  }
  return {};
}

uint32_t
MazeBFS::mid_cell_checksum(const std::vector<MazeGenerator::Cell *> &p) {
  if (p.empty())
    return 0;
  size_t mid = p.size() / 2;
  auto *cell = p[mid];
  return static_cast<uint32_t>(cell->x * cell->y);
}

void MazeBFS::run(int) {
  path = bfs(maze->get_start(), maze->get_finish());
  result_val += static_cast<uint32_t>(path.size());
}

uint32_t MazeBFS::checksum() { return result_val + mid_cell_checksum(path); }

bool MazeAStar::Node::operator>(const Node &other) const {
  if (f_score != other.f_score)
    return f_score > other.f_score;
  return idx > other.idx;
}

MazeAStar::MazeAStar() : result_val(0) {
  width = static_cast<int32_t>(config_val("w"));
  height = static_cast<int32_t>(config_val("h"));
  maze = std::make_unique<MazeGenerator::Maze>(width, height);
}

std::string MazeAStar::name() const { return "Maze::AStar"; }

void MazeAStar::prepare() { maze->generate(); }

int MazeAStar::heuristic(MazeGenerator::Cell *a, MazeGenerator::Cell *b) {
  return std::abs(a->x - b->x) + std::abs(a->y - b->y);
}

int MazeAStar::idx(int y, int x) const { return y * width + x; }

std::vector<MazeGenerator::Cell *>
MazeAStar::astar(MazeGenerator::Cell *start, MazeGenerator::Cell *target) {
  if (start == target)
    return {start};

  int size = width * height;
  std::vector<int> came_from(size, -1);
  std::vector<int> g_score(size, std::numeric_limits<int>::max());
  std::vector<int> best_f(size, std::numeric_limits<int>::max());

  int start_idx = idx(start->y, start->x);
  int target_idx = idx(target->y, target->x);

  std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open_set;
  g_score[start_idx] = 0;
  int f_start = heuristic(start, target);
  open_set.push({f_start, start_idx});
  best_f[start_idx] = f_start;

  while (!open_set.empty()) {
    auto [f_val, current_idx] = open_set.top();
    open_set.pop();
    if (f_val != best_f[current_idx])
      continue;

    if (current_idx == target_idx) {
      std::vector<MazeGenerator::Cell *> result;
      int cur = current_idx;
      while (cur != -1) {
        int y = cur / width, x = cur % width;
        result.push_back(maze->get_cell(x, y));
        cur = came_from[cur];
      }
      std::reverse(result.begin(), result.end());
      return result;
    }

    int current_y = current_idx / width, current_x = current_idx % width;
    auto *current = maze->get_cell(current_x, current_y);
    int current_g = g_score[current_idx];

    for (auto *neighbor : current->neighbors) {
      if (!neighbor->is_walkable())
        continue;
      int neighbor_idx = idx(neighbor->y, neighbor->x);
      int tentative_g = current_g + 1;

      if (tentative_g < g_score[neighbor_idx]) {
        came_from[neighbor_idx] = current_idx;
        g_score[neighbor_idx] = tentative_g;
        int f_new = tentative_g + heuristic(neighbor, target);
        if (f_new < best_f[neighbor_idx]) {
          best_f[neighbor_idx] = f_new;
          open_set.push({f_new, neighbor_idx});
        }
      }
    }
  }
  return {};
}

uint32_t
MazeAStar::mid_cell_checksum(const std::vector<MazeGenerator::Cell *> &p) {
  if (p.empty())
    return 0;
  size_t mid = p.size() / 2;
  auto *cell = p[mid];
  return static_cast<uint32_t>(cell->x * cell->y);
}

void MazeAStar::run(int) {
  path = astar(maze->get_start(), maze->get_finish());
  result_val += static_cast<uint32_t>(path.size());
}

uint32_t MazeAStar::checksum() { return result_val + mid_cell_checksum(path); }