#include "benchmark.h"

typedef struct CacheNode {
  char *key;
  char *value;
  int64_t timestamp;
  struct CacheNode *prev;
  struct CacheNode *next;
  UT_hash_handle hh;
} CacheNode;

typedef struct {
  CacheNode *head;
  CacheNode *tail;
  CacheNode *hash_table;
  size_t capacity;
  size_t size;
  int64_t time;
} CacheSimulationCache;

typedef struct {
  uint32_t result_val;
  int64_t values_size;
  int64_t cache_size;
  int64_t hits;
  int64_t misses;
  CacheSimulationCache *cache;
} CacheSimulationData;

static CacheSimulationCache *cache_simulation_cache_new(size_t capacity) {
  CacheSimulationCache *cache = malloc(sizeof(CacheSimulationCache));
  cache->head = NULL;
  cache->tail = NULL;
  cache->hash_table = NULL;
  cache->capacity = capacity;
  cache->size = 0;
  cache->time = 0;
  return cache;
}

static void cache_simulation_cache_move_to_front(CacheSimulationCache *cache,
                                                 CacheNode *node) {
  if (node == cache->head)
    return;

  if (node->prev)
    node->prev->next = node->next;
  if (node->next)
    node->next->prev = node->prev;

  if (node == cache->tail)
    cache->tail = node->prev;

  node->prev = NULL;
  node->next = cache->head;
  if (cache->head)
    cache->head->prev = node;
  cache->head = node;
  if (!cache->tail)
    cache->tail = node;
}

static CacheNode *cache_simulation_cache_get(CacheSimulationCache *cache,
                                             const char *key) {
  CacheNode *node = NULL;
  HASH_FIND_STR(cache->hash_table, key, node);

  if (node) {
    cache_simulation_cache_move_to_front(cache, node);
    node->timestamp = ++cache->time;
    return node;
  }
  return NULL;
}

static void cache_simulation_cache_remove_oldest(CacheSimulationCache *cache) {
  if (!cache->tail)
    return;

  CacheNode *oldest = cache->tail;

  HASH_DEL(cache->hash_table, oldest);

  if (oldest->prev)
    oldest->prev->next = oldest->next;
  if (oldest->next)
    oldest->next->prev = oldest->prev;

  if (cache->head == oldest)
    cache->head = oldest->next;
  if (cache->tail == oldest)
    cache->tail = oldest->prev;

  free(oldest->key);
  free(oldest->value);
  free(oldest);

  cache->size--;
}

static void cache_simulation_cache_put(CacheSimulationCache *cache,
                                       const char *key, const char *value) {
  CacheNode *node = NULL;
  HASH_FIND_STR(cache->hash_table, key, node);

  if (node) {

    free(node->value);
    node->value = strdup(value);
    cache_simulation_cache_move_to_front(cache, node);
    node->timestamp = ++cache->time;
    return;
  }

  if (cache->size >= cache->capacity) {
    cache_simulation_cache_remove_oldest(cache);
  }

  node = malloc(sizeof(CacheNode));
  node->key = strdup(key);
  node->value = strdup(value);
  node->timestamp = ++cache->time;
  node->prev = NULL;
  node->next = NULL;

  HASH_ADD_STR(cache->hash_table, key, node);

  node->next = cache->head;
  if (cache->head)
    cache->head->prev = node;
  cache->head = node;
  if (!cache->tail)
    cache->tail = node;

  cache->size++;
}

static void cache_simulation_cache_free(CacheSimulationCache *cache) {
  CacheNode *node, *tmp;
  HASH_ITER(hh, cache->hash_table, node, tmp) {
    HASH_DEL(cache->hash_table, node);
    free(node->key);
    free(node->value);
    free(node);
  }
  free(cache);
}

void CacheSimulation_prepare(Benchmark *self) {
  CacheSimulationData *data = (CacheSimulationData *)self->data;

  data->values_size = Helper_config_i64(self->name, "values");
  if (data->values_size <= 0) {
    data->values_size = 2000;
  }

  data->cache_size = Helper_config_i64(self->name, "size");
  if (data->cache_size <= 0) {
    data->cache_size = 1000;
  }

  data->result_val = 5432;
  data->hits = 0;
  data->misses = 0;
  data->cache = cache_simulation_cache_new((size_t)data->cache_size);
}

void CacheSimulation_run(Benchmark *self, int iteration_id) {
  CacheSimulationData *data = (CacheSimulationData *)self->data;

  for (int i = 0; i < 1000; i++) {
    char key_buf[32];
    snprintf(key_buf, sizeof(key_buf), "item_%u",
             Helper_next_int((uint32_t)data->values_size));

    if (cache_simulation_cache_get(data->cache, key_buf)) {
      data->hits++;
      char value_buf[32];
      snprintf(value_buf, sizeof(value_buf), "updated_%d", iteration_id);
      cache_simulation_cache_put(data->cache, key_buf, value_buf);
    } else {
      data->misses++;
      char value_buf[32];
      snprintf(value_buf, sizeof(value_buf), "new_%d", iteration_id);
      cache_simulation_cache_put(data->cache, key_buf, value_buf);
    }
  }
}

uint32_t CacheSimulation_checksum(Benchmark *self) {
  CacheSimulationData *data = (CacheSimulationData *)self->data;

  uint32_t final_result = data->result_val;
  final_result = ((int64_t)final_result << 5) + (uint32_t)data->hits;
  final_result = ((int64_t)final_result << 5) + (uint32_t)data->misses;
  final_result = ((int64_t)final_result << 5) + (uint32_t)data->cache->size;

  return final_result;
}

void CacheSimulation_cleanup(Benchmark *self) {
  CacheSimulationData *data = (CacheSimulationData *)self->data;

  if (data->cache) {
    cache_simulation_cache_free(data->cache);
    data->cache = NULL;
  }
}

Benchmark *CacheSimulation_create(void) {
  Benchmark *bench = Benchmark_create("Etc::CacheSimulation");

  CacheSimulationData *data = malloc(sizeof(CacheSimulationData));
  memset(data, 0, sizeof(CacheSimulationData));

  bench->data = data;

  bench->prepare = CacheSimulation_prepare;
  bench->run = CacheSimulation_run;
  bench->checksum = CacheSimulation_checksum;
  bench->cleanup = CacheSimulation_cleanup;

  return bench;
}
