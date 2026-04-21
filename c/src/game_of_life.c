#include "benchmark.h"

typedef struct Cell Cell;

struct Cell {
  bool alive;
  bool next_state;
  Cell **neighbors;
  int neighbor_count;
};

typedef struct {
  int width;
  int height;
  Cell ***cells;
} GameOfLifeGrid;

typedef struct {
  uint32_t result_val;
  int64_t width_val;
  int64_t height_val;
  GameOfLifeGrid grid;
} GameOfLifeData;

static uint32_t fnv1a_hash(uint32_t hash, uint32_t value) {
  const uint32_t FNV_OFFSET_BASIS = 2166136261UL;
  const uint32_t FNV_PRIME = 16777619UL;

  if (hash == 0)
    hash = FNV_OFFSET_BASIS;
  hash ^= value;
  hash *= FNV_PRIME;
  return hash;
}

static Cell *cell_create(void) {
  Cell *cell = malloc(sizeof(Cell));
  cell->alive = false;
  cell->next_state = false;
  cell->neighbors = malloc(8 * sizeof(Cell *));
  cell->neighbor_count = 0;
  return cell;
}

static void cell_destroy(Cell *cell) {
  free(cell->neighbors);
  free(cell);
}

static void cell_add_neighbor(Cell *cell, Cell *neighbor) {
  cell->neighbors[cell->neighbor_count++] = neighbor;
}

static void cell_compute_next_state(Cell *cell) {
  int alive_neighbors = 0;
  for (int i = 0; i < cell->neighbor_count; i++) {
    if (cell->neighbors[i]->alive) {
      alive_neighbors++;
    }
  }

  if (cell->alive) {
    cell->next_state = (alive_neighbors == 2 || alive_neighbors == 3);
  } else {
    cell->next_state = (alive_neighbors == 3);
  }
}

static void cell_update(Cell *cell) { cell->alive = cell->next_state; }

static void game_of_life_grid_init(GameOfLifeGrid *grid, int width,
                                   int height) {
  grid->width = width;
  grid->height = height;

  grid->cells = malloc(height * sizeof(Cell **));
  for (int y = 0; y < height; y++) {
    grid->cells[y] = malloc(width * sizeof(Cell *));
    for (int x = 0; x < width; x++) {
      grid->cells[y][x] = cell_create();
    }
  }

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      Cell *cell = grid->cells[y][x];

      for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
          if (dx == 0 && dy == 0)
            continue;

          int ny = (y + dy + height) % height;
          int nx = (x + dx + width) % width;

          cell_add_neighbor(cell, grid->cells[ny][nx]);
        }
      }
    }
  }
}

static void game_of_life_grid_free(GameOfLifeGrid *grid) {
  for (int y = 0; y < grid->height; y++) {
    for (int x = 0; x < grid->width; x++) {
      cell_destroy(grid->cells[y][x]);
    }
    free(grid->cells[y]);
  }
  free(grid->cells);
}

static void game_of_life_next_generation(GameOfLifeGrid *grid) {

  for (int y = 0; y < grid->height; y++) {
    for (int x = 0; x < grid->width; x++) {
      cell_compute_next_state(grid->cells[y][x]);
    }
  }

  for (int y = 0; y < grid->height; y++) {
    for (int x = 0; x < grid->width; x++) {
      cell_update(grid->cells[y][x]);
    }
  }
}

static int game_of_life_count_alive(GameOfLifeGrid *grid) {
  int count = 0;
  for (int y = 0; y < grid->height; y++) {
    for (int x = 0; x < grid->width; x++) {
      if (grid->cells[y][x]->alive)
        count++;
    }
  }
  return count;
}

static uint32_t game_of_life_grid_hash(GameOfLifeGrid *grid) {
  uint32_t hash = 0;

  for (int y = 0; y < grid->height; y++) {
    for (int x = 0; x < grid->width; x++) {
      uint32_t alive = (grid->cells[y][x]->alive) ? 1 : 0;
      hash = fnv1a_hash(hash, alive);
    }
  }

  return hash;
}

void GameOfLife_prepare(Benchmark *self) {
  GameOfLifeData *data = (GameOfLifeData *)self->data;

  data->width_val = Helper_config_i64(self->name, "w");
  data->height_val = Helper_config_i64(self->name, "h");

  if (data->width_val <= 0)
    data->width_val = 256;
  if (data->height_val <= 0)
    data->height_val = 256;

  game_of_life_grid_init(&data->grid, (int)data->width_val,
                         (int)data->height_val);

  for (int y = 0; y < data->grid.height; y++) {
    for (int x = 0; x < data->grid.width; x++) {
      if (Helper_next_float(1.0) < 0.1) {
        data->grid.cells[y][x]->alive = true;
      }
    }
  }

  data->result_val = 0;
}

void GameOfLife_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  GameOfLifeData *data = (GameOfLifeData *)self->data;
  game_of_life_next_generation(&data->grid);
}

uint32_t GameOfLife_checksum(Benchmark *self) {
  GameOfLifeData *data = (GameOfLifeData *)self->data;
  int alive = game_of_life_count_alive(&data->grid);
  return game_of_life_grid_hash(&data->grid) + (uint32_t)alive;
}

void GameOfLife_cleanup(Benchmark *self) {
  GameOfLifeData *data = (GameOfLifeData *)self->data;
  game_of_life_grid_free(&data->grid);
}

Benchmark *GameOfLife_create(void) {
  Benchmark *bench = Benchmark_create("Etc::GameOfLife");

  GameOfLifeData *data = malloc(sizeof(GameOfLifeData));
  memset(data, 0, sizeof(GameOfLifeData));

  bench->data = data;
  bench->prepare = GameOfLife_prepare;
  bench->run = GameOfLife_run;
  bench->checksum = GameOfLife_checksum;
  bench->cleanup = GameOfLife_cleanup;

  return bench;
}
