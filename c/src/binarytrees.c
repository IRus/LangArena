#include "benchmark.h"

typedef struct BinarytreesObj_TreeNode {
  struct BinarytreesObj_TreeNode *left;
  struct BinarytreesObj_TreeNode *right;
  int32_t item;
} BinarytreesObj_TreeNode;

static BinarytreesObj_TreeNode *BinarytreesObj_TreeNode_new(int32_t item,
                                                            int32_t depth) {
  BinarytreesObj_TreeNode *node = malloc(sizeof(BinarytreesObj_TreeNode));
  node->item = item;
  node->left = NULL;
  node->right = NULL;

  if (depth > 0) {
    int32_t shift = 1 << (depth - 1);
    node->left = BinarytreesObj_TreeNode_new(item - shift, depth - 1);
    node->right = BinarytreesObj_TreeNode_new(item + shift, depth - 1);
  }

  return node;
}

static void BinarytreesObj_TreeNode_free(BinarytreesObj_TreeNode *node) {
  if (!node)
    return;
  BinarytreesObj_TreeNode_free(node->left);
  BinarytreesObj_TreeNode_free(node->right);
  free(node);
}

static uint32_t BinarytreesObj_TreeNode_sum(BinarytreesObj_TreeNode *node) {
  uint32_t total = (uint32_t)node->item + 1;
  if (node->left)
    total += BinarytreesObj_TreeNode_sum(node->left);
  if (node->right)
    total += BinarytreesObj_TreeNode_sum(node->right);
  return total;
}

typedef struct {
  int64_t n;
  uint32_t result_val;
} BinarytreesObjData;

void BinarytreesObj_prepare(Benchmark *self) {
  BinarytreesObjData *data = (BinarytreesObjData *)self->data;
  data->n = Helper_config_i64(self->name, "depth");
  data->result_val = 0;
}

void BinarytreesObj_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  BinarytreesObjData *data = (BinarytreesObjData *)self->data;

  BinarytreesObj_TreeNode *root =
      BinarytreesObj_TreeNode_new(0, (int32_t)data->n);
  data->result_val += BinarytreesObj_TreeNode_sum(root);
  BinarytreesObj_TreeNode_free(root);
}

uint32_t BinarytreesObj_checksum(Benchmark *self) {
  BinarytreesObjData *data = (BinarytreesObjData *)self->data;
  return data->result_val;
}

void BinarytreesObj_cleanup(Benchmark *self) {
  if (self && self->data) {
    BinarytreesObjData *data = (BinarytreesObjData *)self->data;

    free(data);
    self->data = NULL;
  }
}

Benchmark *BinarytreesObj_create(void) {
  Benchmark *bench = Benchmark_create("Binarytrees::Obj");
  if (!bench)
    return NULL;

  BinarytreesObjData *data = malloc(sizeof(BinarytreesObjData));
  if (!data) {
    free(bench);
    return NULL;
  }

  data->n = 0;
  data->result_val = 0;

  bench->data = data;
  bench->prepare = BinarytreesObj_prepare;
  bench->run = BinarytreesObj_run;
  bench->checksum = BinarytreesObj_checksum;
  bench->cleanup = BinarytreesObj_cleanup;

  return bench;
}

typedef struct {
  int32_t item;
  int32_t left;
  int32_t right;
} BinarytreesArena_Node;

typedef struct {
  BinarytreesArena_Node *nodes;
  size_t capacity;
  size_t size;
} BinarytreesArena_Arena;

static BinarytreesArena_Arena *BinarytreesArena_Arena_new(void) {
  BinarytreesArena_Arena *arena = malloc(sizeof(BinarytreesArena_Arena));
  if (!arena)
    return NULL;

  arena->nodes = NULL;
  arena->capacity = 0;
  arena->size = 0;
  return arena;
}

static void BinarytreesArena_Arena_free(BinarytreesArena_Arena *arena) {
  if (arena) {
    if (arena->nodes) {
      free(arena->nodes);
      arena->nodes = NULL;
    }
    arena->capacity = 0;
    arena->size = 0;
    free(arena);
  }
}

static int32_t BinarytreesArena_Arena_add(BinarytreesArena_Arena *arena,
                                          int32_t item) {
  if (!arena)
    return -1;

  if (arena->size >= arena->capacity) {
    size_t new_capacity = arena->capacity == 0 ? 1024 : arena->capacity * 2;
    BinarytreesArena_Node *new_nodes =
        realloc(arena->nodes, new_capacity * sizeof(BinarytreesArena_Node));
    if (!new_nodes)
      return -1;

    arena->nodes = new_nodes;
    arena->capacity = new_capacity;
  }

  int32_t idx = (int32_t)arena->size;
  arena->nodes[idx].item = item;
  arena->nodes[idx].left = -1;
  arena->nodes[idx].right = -1;
  arena->size++;

  return idx;
}

static int32_t BinarytreesArena_build_tree(BinarytreesArena_Arena *arena,
                                           int32_t item, int32_t depth) {
  if (!arena)
    return -1;

  int32_t idx = BinarytreesArena_Arena_add(arena, item);
  if (idx < 0)
    return -1;

  if (depth > 0) {
    int32_t shift = 1 << (depth - 1);
    int32_t left_idx =
        BinarytreesArena_build_tree(arena, item - shift, depth - 1);
    int32_t right_idx =
        BinarytreesArena_build_tree(arena, item + shift, depth - 1);

    if (left_idx >= 0 && right_idx >= 0) {
      arena->nodes[idx].left = left_idx;
      arena->nodes[idx].right = right_idx;
    }
  }

  return idx;
}

static uint32_t BinarytreesArena_sum(BinarytreesArena_Arena *arena,
                                     int32_t idx) {
  if (!arena || idx < 0 || (size_t)idx >= arena->size)
    return 0;

  BinarytreesArena_Node *node = &arena->nodes[idx];
  uint32_t total = (uint32_t)node->item + 1;

  if (node->left >= 0)
    total += BinarytreesArena_sum(arena, node->left);
  if (node->right >= 0)
    total += BinarytreesArena_sum(arena, node->right);

  return total;
}

typedef struct {
  int64_t n;
  uint32_t result_val;
  BinarytreesArena_Arena *arena;
} BinarytreesArenaData;

void BinarytreesArena_prepare(Benchmark *self) {
  if (!self || !self->data)
    return;

  BinarytreesArenaData *data = (BinarytreesArenaData *)self->data;
  data->n = Helper_config_i64(self->name, "depth");
  data->result_val = 0;
}

void BinarytreesArena_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  if (!self || !self->data)
    return;

  BinarytreesArenaData *data = (BinarytreesArenaData *)self->data;

  data->arena = BinarytreesArena_Arena_new();

  int32_t root_idx =
      BinarytreesArena_build_tree(data->arena, 0, (int32_t)data->n);
  if (root_idx >= 0) {
    data->result_val += BinarytreesArena_sum(data->arena, root_idx);
  }

  BinarytreesArena_Arena_free(data->arena);
  data->arena = NULL;
}

uint32_t BinarytreesArena_checksum(Benchmark *self) {
  if (!self || !self->data)
    return 0;
  BinarytreesArenaData *data = (BinarytreesArenaData *)self->data;
  return data->result_val;
}

void BinarytreesArena_cleanup(Benchmark *self) {
  if (self && self->data) {
    BinarytreesArenaData *data = (BinarytreesArenaData *)self->data;

    if (data->arena) {
      BinarytreesArena_Arena_free(data->arena);
      data->arena = NULL;
    }

    free(data);
    self->data = NULL;
  }
}

Benchmark *BinarytreesArena_create(void) {
  Benchmark *bench = Benchmark_create("Binarytrees::Arena");
  if (!bench)
    return NULL;

  BinarytreesArenaData *data = malloc(sizeof(BinarytreesArenaData));
  if (!data) {
    free(bench);
    return NULL;
  }

  data->n = 0;
  data->result_val = 0;
  data->arena = NULL;

  bench->data = data;
  bench->prepare = BinarytreesArena_prepare;
  bench->run = BinarytreesArena_run;
  bench->checksum = BinarytreesArena_checksum;
  bench->cleanup = BinarytreesArena_cleanup;

  return bench;
}
