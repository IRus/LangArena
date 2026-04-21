#include "benchmark.h"

typedef enum {
  MAZE_CELL_WALL = 0,
  MAZE_CELL_SPACE,
  MAZE_CELL_START,
  MAZE_CELL_FINISH,
  MAZE_CELL_BORDER,
  MAZE_CELL_PATH
} MazeCellKind;

typedef struct MazeCell {
  MazeCellKind kind;
  struct MazeCell *neighbors[4];
  int neighbor_count;
  int x;
  int y;
} MazeCell;

typedef struct {
  int width;
  int height;
  MazeCell **cells;
  MazeCell *start;
  MazeCell *finish;
} Maze;

static MazeCell *maze_cell_create(int x, int y) {
  MazeCell *cell = malloc(sizeof(MazeCell));
  cell->kind = MAZE_CELL_WALL;
  cell->x = x;
  cell->y = y;
  cell->neighbor_count = 0;
  return cell;
}

static void maze_cell_add_neighbor(MazeCell *cell, MazeCell *neighbor) {
  if (cell->neighbor_count < 4) {
    cell->neighbors[cell->neighbor_count++] = neighbor;
  }
}

static bool maze_cell_is_walkable(MazeCell *cell) {
  return cell->kind == MAZE_CELL_SPACE || cell->kind == MAZE_CELL_START ||
         cell->kind == MAZE_CELL_FINISH;
}

static void maze_cell_reset(MazeCell *cell) {
  if (cell->kind == MAZE_CELL_SPACE) {
    cell->kind = MAZE_CELL_WALL;
  }
}

static Maze *maze_create(int width, int height) {
  Maze *maze = malloc(sizeof(Maze));
  maze->width = width;
  maze->height = height;

  maze->cells = malloc(height * sizeof(MazeCell *));
  for (int y = 0; y < height; y++) {
    maze->cells[y] = malloc(width * sizeof(MazeCell));
    for (int x = 0; x < width; x++) {
      maze->cells[y][x] = *maze_cell_create(x, y);
    }
  }

  maze->start = &maze->cells[1][1];
  maze->finish = &maze->cells[height - 2][width - 2];
  maze->start->kind = MAZE_CELL_START;
  maze->finish->kind = MAZE_CELL_FINISH;

  return maze;
}

static void maze_update_neighbors(Maze *maze) {
  for (int y = 0; y < maze->height; y++) {
    for (int x = 0; x < maze->width; x++) {
      MazeCell *cell = &maze->cells[y][x];
      cell->neighbor_count = 0;

      if (x > 0 && y > 0 && x < maze->width - 1 && y < maze->height - 1) {
        maze_cell_add_neighbor(cell, &maze->cells[y - 1][x]);
        maze_cell_add_neighbor(cell, &maze->cells[y + 1][x]);
        maze_cell_add_neighbor(cell, &maze->cells[y][x + 1]);
        maze_cell_add_neighbor(cell, &maze->cells[y][x - 1]);

        for (int t = 0; t < 4; t++) {
          int i = Helper_next_int(4);
          int j = Helper_next_int(4);
          if (i != j) {
            MazeCell *temp = cell->neighbors[i];
            cell->neighbors[i] = cell->neighbors[j];
            cell->neighbors[j] = temp;
          }
        }
      } else {
        cell->kind = MAZE_CELL_BORDER;
      }
    }
  }
}

static void maze_reset(Maze *maze) {
  for (int y = 0; y < maze->height; y++) {
    for (int x = 0; x < maze->width; x++) {
      maze_cell_reset(&maze->cells[y][x]);
    }
  }
  maze->start->kind = MAZE_CELL_START;
  maze->finish->kind = MAZE_CELL_FINISH;
}

static void maze_dig(Maze *maze, MazeCell *start_cell) {
  size_t max_size = maze->width * maze->height;
  MazeCell **stack = malloc(max_size * sizeof(MazeCell *));

  if (!stack)
    return;

  size_t stack_size = 0;
  stack[stack_size++] = start_cell;

  while (stack_size > 0) {
    MazeCell *cell = stack[--stack_size];

    int walkable = 0;
    for (int i = 0; i < cell->neighbor_count; i++) {
      if (maze_cell_is_walkable(cell->neighbors[i]))
        walkable++;
    }

    if (walkable != 1)
      continue;

    cell->kind = MAZE_CELL_SPACE;

    for (int i = 0; i < cell->neighbor_count; i++) {
      MazeCell *n = cell->neighbors[i];
      if (n->kind == MAZE_CELL_WALL) {

        if (stack_size >= max_size) {

          max_size *= 2;
          MazeCell **new_stack = realloc(stack, max_size * sizeof(MazeCell *));
          if (!new_stack) {

            free(stack);
            return;
          }
          stack = new_stack;
        }
        stack[stack_size++] = n;
      }
    }
  }

  free(stack);
}
static void maze_ensure_open_finish(Maze *maze, MazeCell *start_cell) {
  MazeCell **stack = malloc(maze->width * maze->height * sizeof(MazeCell *));
  int stack_size = 0;
  stack[stack_size++] = start_cell;

  while (stack_size > 0) {
    MazeCell *cell = stack[--stack_size];

    cell->kind = MAZE_CELL_SPACE;

    int walkable = 0;
    for (int i = 0; i < cell->neighbor_count; i++) {
      if (maze_cell_is_walkable(cell->neighbors[i]))
        walkable++;
    }

    if (walkable > 1)
      continue;

    for (int i = 0; i < cell->neighbor_count; i++) {
      MazeCell *n = cell->neighbors[i];
      if (n->kind == MAZE_CELL_WALL) {
        stack[stack_size++] = n;
      }
    }
  }

  free(stack);
}

static void maze_generate(Maze *maze) {
  for (int i = 0; i < maze->start->neighbor_count; i++) {
    MazeCell *n = maze->start->neighbors[i];
    if (n->kind == MAZE_CELL_WALL) {
      maze_dig(maze, n);
    }
  }

  for (int i = 0; i < maze->finish->neighbor_count; i++) {
    MazeCell *n = maze->finish->neighbors[i];
    if (n->kind == MAZE_CELL_WALL) {
      maze_ensure_open_finish(maze, n);
    }
  }
}

static MazeCell *maze_middle_cell(Maze *maze) {
  return &maze->cells[maze->height / 2][maze->width / 2];
}

static uint32_t maze_checksum(Maze *maze) {
  uint32_t hasher = 2166136261UL;
  uint32_t prime = 16777619UL;

  for (int y = 0; y < maze->height; y++) {
    for (int x = 0; x < maze->width; x++) {
      if (maze->cells[y][x].kind == MAZE_CELL_SPACE) {
        uint32_t val = (uint32_t)(x * y);
        hasher = (hasher ^ val) * prime;
      }
    }
  }
  return hasher;
}

static void maze_free(Maze *maze) {
  if (!maze)
    return;
  for (int y = 0; y < maze->height; y++) {
    free(maze->cells[y]);
  }
  free(maze->cells);
  free(maze);
}

typedef struct {
  uint32_t result_val;
  int width;
  int height;
  Maze *maze;
} MazeGeneratorData;

void MazeGenerator_prepare(Benchmark *self) {
  MazeGeneratorData *data = (MazeGeneratorData *)self->data;

  data->width = (int)Helper_config_i64(self->name, "w");
  data->height = (int)Helper_config_i64(self->name, "h");

  if (data->width < 5)
    data->width = 5;
  if (data->height < 5)
    data->height = 5;

  data->maze = maze_create(data->width, data->height);
  maze_update_neighbors(data->maze);
  data->result_val = 0;
}

void MazeGenerator_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  MazeGeneratorData *data = (MazeGeneratorData *)self->data;

  maze_reset(data->maze);
  maze_generate(data->maze);

  data->result_val += maze_middle_cell(data->maze)->kind;
}

uint32_t MazeGenerator_checksum(Benchmark *self) {
  MazeGeneratorData *data = (MazeGeneratorData *)self->data;
  return data->result_val + maze_checksum(data->maze);
}

void MazeGenerator_cleanup(Benchmark *self) {
  MazeGeneratorData *data = (MazeGeneratorData *)self->data;
  if (data->maze) {
    maze_free(data->maze);
    data->maze = NULL;
  }
}

Benchmark *MazeGenerator_create(void) {
  Benchmark *bench = Benchmark_create("Maze::Generator");
  MazeGeneratorData *data = calloc(1, sizeof(MazeGeneratorData));
  bench->data = data;
  bench->prepare = MazeGenerator_prepare;
  bench->run = MazeGenerator_run;
  bench->checksum = MazeGenerator_checksum;
  bench->cleanup = MazeGenerator_cleanup;
  return bench;
}

typedef struct PathNode {
  MazeCell *cell;
  int parent;
} PathNode;

typedef struct {
  uint32_t result_val;
  int width;
  int height;
  Maze *maze;
  MazeCell **path;
  int path_length;
} MazeBFSData;

void MazeBFS_prepare(Benchmark *self) {
  MazeBFSData *data = (MazeBFSData *)self->data;

  data->width = (int)Helper_config_i64(self->name, "w");
  data->height = (int)Helper_config_i64(self->name, "h");

  if (data->width < 5)
    data->width = 5;
  if (data->height < 5)
    data->height = 5;

  data->maze = maze_create(data->width, data->height);
  maze_update_neighbors(data->maze);
  maze_generate(data->maze);
  data->result_val = 0;
  data->path = NULL;
  data->path_length = 0;
}

static MazeCell **maze_bfs(Maze *maze, MazeCell *start, MazeCell *target,
                           int *out_length) {
  if (start == target) {
    *out_length = 1;
    MazeCell **result = malloc(sizeof(MazeCell *));
    result[0] = start;
    return result;
  }

  int width = maze->width;
  int height = maze->height;

  int *queue = malloc(width * height * sizeof(int));
  int queue_head = 0;
  int queue_tail = 0;

  bool **visited = malloc(height * sizeof(bool *));
  for (int y = 0; y < height; y++) {
    visited[y] = calloc(width, sizeof(bool));
  }

  PathNode *path_nodes = malloc(width * height * sizeof(PathNode));
  int path_count = 0;

  visited[start->y][start->x] = true;
  path_nodes[path_count].cell = start;
  path_nodes[path_count].parent = -1;
  path_count++;
  queue[queue_tail++] = 0;

  while (queue_head < queue_tail) {
    int path_id = queue[queue_head++];
    MazeCell *cell = path_nodes[path_id].cell;

    for (int i = 0; i < cell->neighbor_count; i++) {
      MazeCell *neighbor = cell->neighbors[i];

      if (neighbor == target) {

        int length = 1;
        int cur = path_id;
        while (cur >= 0) {
          length++;
          cur = path_nodes[cur].parent;
        }

        MazeCell **result = malloc(length * sizeof(MazeCell *));
        int idx = length - 1;
        result[idx--] = target;

        cur = path_id;
        while (cur >= 0) {
          result[idx--] = path_nodes[cur].cell;
          cur = path_nodes[cur].parent;
        }

        *out_length = length;

        free(queue);
        for (int y = 0; y < height; y++)
          free(visited[y]);
        free(visited);
        free(path_nodes);

        return result;
      }

      if (maze_cell_is_walkable(neighbor) &&
          !visited[neighbor->y][neighbor->x]) {
        visited[neighbor->y][neighbor->x] = true;
        path_nodes[path_count].cell = neighbor;
        path_nodes[path_count].parent = path_id;
        queue[queue_tail++] = path_count;
        path_count++;
      }
    }
  }

  *out_length = 0;

  free(queue);
  for (int y = 0; y < height; y++)
    free(visited[y]);
  free(visited);
  free(path_nodes);

  return NULL;
}

static uint32_t mid_cell_checksum(MazeCell **path, int length) {
  if (length == 0)
    return 0;
  int mid = length / 2;
  MazeCell *cell = path[mid];
  return (uint32_t)(cell->x * cell->y);
}

void MazeBFS_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  MazeBFSData *data = (MazeBFSData *)self->data;

  if (data->path) {
    free(data->path);
    data->path = NULL;
  }

  data->path = maze_bfs(data->maze, data->maze->start, data->maze->finish,
                        &data->path_length);
  data->result_val += data->path_length;
}

uint32_t MazeBFS_checksum(Benchmark *self) {
  MazeBFSData *data = (MazeBFSData *)self->data;
  return data->result_val + mid_cell_checksum(data->path, data->path_length);
}

void MazeBFS_cleanup(Benchmark *self) {
  MazeBFSData *data = (MazeBFSData *)self->data;
  if (data->path)
    free(data->path);
  if (data->maze)
    maze_free(data->maze);
}

Benchmark *MazeBFS_create(void) {
  Benchmark *bench = Benchmark_create("Maze::BFS");
  MazeBFSData *data = calloc(1, sizeof(MazeBFSData));
  bench->data = data;
  bench->prepare = MazeBFS_prepare;
  bench->run = MazeBFS_run;
  bench->checksum = MazeBFS_checksum;
  bench->cleanup = MazeBFS_cleanup;
  return bench;
}

typedef struct {
  int priority;
  int vertex;
} AStarPriorityQueueEntry;

typedef struct {
  AStarPriorityQueueEntry *heap;
  int *best_priority;
  int size;
  int capacity;
} AStarPriorityQueue;

static AStarPriorityQueue *astar_pq_create(int capacity) {
  AStarPriorityQueue *pq = malloc(sizeof(AStarPriorityQueue));
  pq->heap = malloc(capacity * sizeof(AStarPriorityQueueEntry));
  pq->best_priority = malloc(capacity * sizeof(int));
  for (int i = 0; i < capacity; i++) {
    pq->best_priority[i] = INT_MAX;
  }
  pq->size = 0;
  pq->capacity = capacity;
  return pq;
}

static void astar_pq_push(AStarPriorityQueue *pq, int vertex, int priority) {
  if (priority >= pq->best_priority[vertex])
    return;
  pq->best_priority[vertex] = priority;

  if (pq->size >= pq->capacity) {
    pq->capacity *= 2;
    pq->heap =
        realloc(pq->heap, pq->capacity * sizeof(AStarPriorityQueueEntry));
  }

  int i = pq->size++;
  while (i > 0) {
    int parent = (i - 1) / 2;
    if (pq->heap[parent].priority <= priority)
      break;
    pq->heap[i] = pq->heap[parent];
    i = parent;
  }
  pq->heap[i].priority = priority;
  pq->heap[i].vertex = vertex;
}

static AStarPriorityQueueEntry astar_pq_pop(AStarPriorityQueue *pq) {
  AStarPriorityQueueEntry min = pq->heap[0];
  pq->size--;

  if (pq->size > 0) {
    AStarPriorityQueueEntry last = pq->heap[pq->size];
    int i = 0;
    while (1) {
      int left = 2 * i + 1;
      int right = 2 * i + 2;
      int smallest = i;

      if (left < pq->size &&
          pq->heap[left].priority < pq->heap[smallest].priority) {
        smallest = left;
      }
      if (right < pq->size &&
          pq->heap[right].priority < pq->heap[smallest].priority) {
        smallest = right;
      }

      if (smallest == i)
        break;

      pq->heap[i] = pq->heap[smallest];
      i = smallest;
    }
    pq->heap[i] = last;
  }

  return min;
}

static bool astar_pq_empty(AStarPriorityQueue *pq) { return pq->size == 0; }

static void astar_pq_free(AStarPriorityQueue *pq) {
  free(pq->heap);
  free(pq->best_priority);
  free(pq);
}

typedef struct {
  uint32_t result_val;
  int width;
  int height;
  Maze *maze;
  MazeCell **path;
  int path_length;
} MazeAStarData;

static int astar_heuristic(MazeCell *a, MazeCell *b) {
  return abs(a->x - b->x) + abs(a->y - b->y);
}

static int astar_idx(int y, int x, int width) { return y * width + x; }

static MazeCell **maze_astar(Maze *maze, MazeCell *start, MazeCell *target,
                             int *out_length) {
  if (start == target) {
    *out_length = 1;
    MazeCell **result = malloc(sizeof(MazeCell *));
    result[0] = start;
    return result;
  }

  int width = maze->width;
  int height = maze->height;
  int size = width * height;

  int *came_from = malloc(size * sizeof(int));
  int *g_score = malloc(size * sizeof(int));
  for (int i = 0; i < size; i++) {
    came_from[i] = -1;
    g_score[i] = INT_MAX;
  }

  int start_idx = astar_idx(start->y, start->x, width);
  int target_idx = astar_idx(target->y, target->x, width);

  AStarPriorityQueue *open_set = astar_pq_create(size);

  g_score[start_idx] = 0;
  astar_pq_push(open_set, start_idx, astar_heuristic(start, target));

  while (!astar_pq_empty(open_set)) {
    AStarPriorityQueueEntry entry = astar_pq_pop(open_set);
    int current_idx = entry.vertex;

    if (current_idx == target_idx) {

      int length = 0;
      int cur = current_idx;
      while (cur != -1) {
        length++;
        cur = came_from[cur];
      }

      MazeCell **result = malloc(length * sizeof(MazeCell *));
      cur = current_idx;
      for (int i = length - 1; i >= 0; i--) {
        int y = cur / width;
        int x = cur % width;
        result[i] = &maze->cells[y][x];
        cur = came_from[cur];
      }

      *out_length = length;

      free(came_from);
      free(g_score);
      astar_pq_free(open_set);

      return result;
    }

    int current_y = current_idx / width;
    int current_x = current_idx % width;
    MazeCell *current = &maze->cells[current_y][current_x];
    int current_g = g_score[current_idx];

    for (int i = 0; i < current->neighbor_count; i++) {
      MazeCell *neighbor = current->neighbors[i];
      if (!maze_cell_is_walkable(neighbor))
        continue;

      int neighbor_idx = astar_idx(neighbor->y, neighbor->x, width);
      int tentative_g = current_g + 1;

      if (tentative_g < g_score[neighbor_idx]) {
        came_from[neighbor_idx] = current_idx;
        g_score[neighbor_idx] = tentative_g;
        int f_new = tentative_g + astar_heuristic(neighbor, target);
        astar_pq_push(open_set, neighbor_idx, f_new);
      }
    }
  }

  *out_length = 0;

  free(came_from);
  free(g_score);
  astar_pq_free(open_set);

  return NULL;
}

void MazeAStar_prepare(Benchmark *self) {
  MazeAStarData *data = (MazeAStarData *)self->data;

  data->width = (int)Helper_config_i64(self->name, "w");
  data->height = (int)Helper_config_i64(self->name, "h");

  if (data->width < 5)
    data->width = 5;
  if (data->height < 5)
    data->height = 5;

  data->maze = maze_create(data->width, data->height);
  maze_update_neighbors(data->maze);
  maze_generate(data->maze);
  data->result_val = 0;
  data->path = NULL;
  data->path_length = 0;
}

void MazeAStar_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  MazeAStarData *data = (MazeAStarData *)self->data;

  if (data->path) {
    free(data->path);
    data->path = NULL;
  }

  data->path = maze_astar(data->maze, data->maze->start, data->maze->finish,
                          &data->path_length);
  data->result_val += data->path_length;
}

uint32_t MazeAStar_checksum(Benchmark *self) {
  MazeAStarData *data = (MazeAStarData *)self->data;
  if (data->path_length == 0)
    return data->result_val;
  int mid = data->path_length / 2;
  MazeCell *cell = data->path[mid];
  return data->result_val + (uint32_t)(cell->x * cell->y);
}

void MazeAStar_cleanup(Benchmark *self) {
  MazeAStarData *data = (MazeAStarData *)self->data;
  if (data->path)
    free(data->path);
  if (data->maze)
    maze_free(data->maze);
}

Benchmark *MazeAStar_create(void) {
  Benchmark *bench = Benchmark_create("Maze::AStar");
  MazeAStarData *data = calloc(1, sizeof(MazeAStarData));
  bench->data = data;
  bench->prepare = MazeAStar_prepare;
  bench->run = MazeAStar_run;
  bench->checksum = MazeAStar_checksum;
  bench->cleanup = MazeAStar_cleanup;
  return bench;
}
