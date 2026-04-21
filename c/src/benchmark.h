#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "helper.h"
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <uthash.h>

typedef struct Benchmark Benchmark;

struct Benchmark {
  const char *name;
  uint32_t checksum_val;
  int64_t iterations_val;

  void (*prepare)(Benchmark *self);
  void (*run)(Benchmark *self, int iteration_id);
  void (*run_all)(Benchmark *self);
  void (*warmup)(Benchmark *self);
  uint32_t (*checksum)(Benchmark *self);
  int64_t (*iterations)(Benchmark *self);
  int64_t (*expected_checksum)(Benchmark *self);
  void (*cleanup)(Benchmark *self);

  void *data;
};

void Benchmark_default_prepare(Benchmark *self);
void Benchmark_default_warmup(Benchmark *self);
void Benchmark_default_run_all(Benchmark *self);
uint32_t Benchmark_default_checksum(Benchmark *self);
int64_t Benchmark_default_iterations(Benchmark *self);
int64_t Benchmark_default_expected_checksum(Benchmark *self);
void Benchmark_default_cleanup(Benchmark *self);
Benchmark *Benchmark_create(const char *name);

typedef struct {
  char name[100];
  Benchmark *(*create)(void);
} BenchmarkFactory;

extern BenchmarkFactory *benchmark_factories;
extern size_t benchmark_factories_count;
extern size_t benchmark_factories_capacity;

void Benchmark_register(const char *name, Benchmark *(*factory)(void));
void Benchmark_all(const char *single_bench);

#endif
