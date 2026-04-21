#include "benchmark.h"

void Benchmark_default_prepare(Benchmark *self) { (void)self; }

void Benchmark_default_warmup(Benchmark *self) {
  int64_t warmup_iters = Helper_config_i64(self->name, "warmup_iterations");
  if (warmup_iters == 0) {
    warmup_iters = self->iterations(self);
    warmup_iters = (int64_t)(warmup_iters * 0.2);
    if (warmup_iters < 1)
      warmup_iters = 1;
  }

  for (int64_t i = 0; i < warmup_iters; i++) {
    self->run(self, i);
  }
}

void Benchmark_default_run_all(Benchmark *self) {
  int64_t iters = self->iterations(self);
  for (int64_t i = 0; i < iters; i++) {
    self->run(self, i);
  }
}

uint32_t Benchmark_default_checksum(Benchmark *self) {
  return self->checksum_val;
}

int64_t Benchmark_default_iterations(Benchmark *self) {
  if (self->iterations_val > 0) {
    return self->iterations_val;
  }
  self->iterations_val = Helper_config_i64(self->name, "iterations");
  return self->iterations_val;
}

int64_t Benchmark_default_expected_checksum(Benchmark *self) {
  return Helper_config_i64(self->name, "checksum");
}

void Benchmark_default_cleanup(Benchmark *self) { (void)self; }

Benchmark *Benchmark_create(const char *name) {
  Benchmark *bench = malloc(sizeof(Benchmark));
  bench->name = name;
  bench->checksum_val = 0;
  bench->iterations_val = 0;
  bench->data = NULL;

  bench->prepare = Benchmark_default_prepare;
  bench->warmup = Benchmark_default_warmup;
  bench->run_all = Benchmark_default_run_all;
  bench->checksum = Benchmark_default_checksum;
  bench->iterations = Benchmark_default_iterations;
  bench->expected_checksum = Benchmark_default_expected_checksum;
  bench->cleanup = Benchmark_default_cleanup;

  return bench;
}

BenchmarkFactory *benchmark_factories = NULL;
size_t benchmark_factories_count = 0;
size_t benchmark_factories_capacity = 0;

void Benchmark_register(const char *name, Benchmark *(*factory)(void)) {
  if (benchmark_factories_count >= benchmark_factories_capacity) {
    benchmark_factories_capacity =
        benchmark_factories_capacity ? benchmark_factories_capacity * 2 : 16;
    benchmark_factories =
        realloc(benchmark_factories,
                sizeof(BenchmarkFactory) * benchmark_factories_capacity);
  }

  strncpy(benchmark_factories[benchmark_factories_count].name, name,
          sizeof(benchmark_factories[benchmark_factories_count].name) - 1);
  benchmark_factories[benchmark_factories_count]
      .name[sizeof(benchmark_factories[benchmark_factories_count].name) - 1] =
      '\0';
  benchmark_factories[benchmark_factories_count].create = factory;
  benchmark_factories_count++;
}

void Benchmark_all(const char *single_bench) {
  struct timespec start, end;
  double summary_time = 0.0;
  int ok = 0;
  int fails = 0;

  for (size_t i = 0; i < global_order_count; i++) {
    const char *bench_name = global_order[i];

    if (single_bench && strlen(single_bench) > 0) {
      const char *haystack = bench_name;
      const char *needle = single_bench;

      char haystack_lower[100];
      char needle_lower[100];
      strncpy(haystack_lower, haystack, sizeof(haystack_lower) - 1);
      haystack_lower[sizeof(haystack_lower) - 1] = '\0';
      strncpy(needle_lower, needle, sizeof(needle_lower) - 1);
      needle_lower[sizeof(needle_lower) - 1] = '\0';

      for (char *p = haystack_lower; *p; p++)
        *p = tolower(*p);
      for (char *p = needle_lower; *p; p++)
        *p = tolower(*p);

      if (strstr(haystack_lower, needle_lower) == NULL) {
        continue;
      }
    }

    BenchmarkFactory *factory = NULL;
    for (size_t j = 0; j < benchmark_factories_count; j++) {
      if (strcmp(benchmark_factories[j].name, bench_name) == 0) {
        factory = &benchmark_factories[j];
        break;
      }
    }

    if (!factory) {
      printf(
          "Warning: Benchmark '%s' defined in config but not found in code\n",
          bench_name);
      continue;
    }

    printf("%s: ", bench_name);
    fflush(stdout);

    Benchmark *bench = factory->create();

    Helper_reset();

    bench->prepare(bench);

    bench->warmup(bench);

    Helper_reset();
    clock_gettime(CLOCK_MONOTONIC, &start);

    bench->run_all(bench);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double duration =
        (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;

    summary_time += duration;

    uint32_t actual_checksum = bench->checksum(bench);
    uint32_t expected_checksum = (uint32_t)bench->expected_checksum(bench);

    if (actual_checksum == expected_checksum) {
      printf("OK ");
      ok++;
    } else {
      printf("ERR[actual=%u, expected=%u] ", actual_checksum,
             (unsigned int)expected_checksum);
      fails++;
    }

    printf("in %.3fs\n", duration);

    bench->cleanup(bench);
    free(bench->data);
    free(bench);

    usleep(1000);
  }

  if (ok + fails > 0) {
    printf("Summary: %.4fs, %d, %d, %d\n", summary_time, ok + fails, ok, fails);
  }

  if (fails > 0) {
    exit(1);
  }
}
