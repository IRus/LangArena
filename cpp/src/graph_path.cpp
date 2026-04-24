#include "graph_path.hpp"
#include <climits>
#include <queue>
#include <stack>

GraphPathBenchmark::Graph::Graph(int vertices, int jumps, int jump_len)
    : vertices(vertices), jumps(jumps), jump_len(jump_len), adj(vertices) {}

void GraphPathBenchmark::Graph::add_edge(int u, int v) {
  adj[u].push_back(v);
  adj[v].push_back(u);
}

void GraphPathBenchmark::Graph::generate_random() {
  for (int i = 1; i < vertices; i++) {
    add_edge(i, i - 1);
  }

  for (int v = 0; v < vertices; v++) {
    int num_jumps = Helper::next_int(jumps);
    for (int j = 0; j < num_jumps; j++) {
      int offset = Helper::next_int(jump_len) - jump_len / 2;
      int u = v + offset;

      if (u >= 0 && u < vertices && u != v) {
        add_edge(v, u);
      }
    }
  }
}

GraphPathBenchmark::GraphPathBenchmark() : result_val(0) {}

void GraphPathBenchmark::prepare() {
  int vertices = static_cast<int>(config_val("vertices"));
  int jumps = static_cast<int>(config_val("jumps"));
  int jump_len = static_cast<int>(config_val("jump_len"));
  graph = std::make_unique<Graph>(vertices, jumps, jump_len);
  graph->generate_random();
}

void GraphPathBenchmark::run(int iteration_id) {
  (void)iteration_id;
  result_val += test();
}

uint32_t GraphPathBenchmark::checksum() { return result_val; }

std::string GraphPathBFS::name() const { return "Graph::BFS"; }

int GraphPathBFS::bfs_shortest_path(int start, int target) {
  if (start == target)
    return 0;

  std::vector<uint8_t> visited(graph->vertices, 0);
  std::queue<std::pair<int, int>> queue;

  visited[start] = 1;
  queue.push({start, 0});

  while (!queue.empty()) {
    auto [v, dist] = queue.front();
    queue.pop();

    for (int neighbor : graph->adj[v]) {
      if (neighbor == target)
        return dist + 1;
      if (visited[neighbor] == 0) {
        visited[neighbor] = 1;
        queue.push({neighbor, dist + 1});
      }
    }
  }
  return -1;
}

int64_t GraphPathBFS::test() {
  return bfs_shortest_path(0, graph->vertices - 1);
}

std::string GraphPathDFS::name() const { return "Graph::DFS"; }

int GraphPathDFS::dfs_shortest_path(int start, int target) {
  if (start == target)
    return 0;

  std::vector<uint8_t> visited(graph->vertices, 0);
  std::stack<std::pair<int, int>> stack;
  int best_path = INT_MAX;

  stack.push({start, 0});

  while (!stack.empty()) {
    auto [v, dist] = stack.top();
    stack.pop();

    if (visited[v] == 1 || dist >= best_path)
      continue;
    visited[v] = 1;

    for (int neighbor : graph->adj[v]) {
      if (neighbor == target) {
        if (dist + 1 < best_path)
          best_path = dist + 1;
      } else if (visited[neighbor] == 0) {
        stack.push({neighbor, dist + 1});
      }
    }
  }
  return (best_path == INT_MAX) ? -1 : best_path;
}

int64_t GraphPathDFS::test() {
  return dfs_shortest_path(0, graph->vertices - 1);
}

std::string GraphPathAStar::name() const { return "Graph::AStar"; }

int GraphPathAStar::astar_shortest_path(int start, int target) {
  if (start == target)
    return 0;

  std::vector<int> g_score(graph->vertices, INT_MAX);
  g_score[start] = 0;

  using QueueType =
      std::priority_queue<Node, std::vector<Node>, std::greater<Node>>;
  QueueType open_set;
  open_set.push({start, heuristic(start, target)});

  std::vector<bool> in_open_set(graph->vertices, false);
  in_open_set[start] = true;

  std::vector<bool> closed(graph->vertices, false);

  while (!open_set.empty()) {
    Node current = open_set.top();
    open_set.pop();

    if (closed[current.vertex])
      continue;
    closed[current.vertex] = true;
    in_open_set[current.vertex] = false;

    if (current.vertex == target)
      return g_score[current.vertex];

    for (int neighbor : graph->adj[current.vertex]) {
      if (closed[neighbor])
        continue;

      int tentative_g = g_score[current.vertex] + 1;

      if (tentative_g < g_score[neighbor]) {
        g_score[neighbor] = tentative_g;
        int f = tentative_g + heuristic(neighbor, target);

        if (!in_open_set[neighbor]) {
          open_set.push({neighbor, f});
          in_open_set[neighbor] = true;
        }
      }
    }
  }
  return -1;
}

int64_t GraphPathAStar::test() {
  return astar_shortest_path(0, graph->vertices - 1);
}