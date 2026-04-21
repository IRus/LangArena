#include "benchmark.h"

typedef struct {
  int64_t n;
  uint32_t result_val;
} FannkuchreduxData;

static void fannkuchredux_swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

static void fannkuchredux_calculate(int n, int *checksum, int *max_flips) {

  int perm1[32];
  int perm[32];
  int count[32];

  if (n > 32)
    n = 32;

  for (int i = 0; i < n; i++)
    perm1[i] = i;

  *max_flips = 0;
  *checksum = 0;
  int permCount = 0;
  int r = n;

  while (1) {
    while (r > 1) {
      count[r - 1] = r;
      r--;
    }

    memcpy(perm, perm1, n * sizeof(int));
    int flipsCount = 0;

    int k = perm[0];
    while (k != 0) {
      int k2 = (k + 1) >> 1;
      for (int i = 0; i < k2; i++) {
        int j = k - i;
        fannkuchredux_swap(&perm[i], &perm[j]);
      }
      flipsCount++;
      k = perm[0];
    }

    if (flipsCount > *max_flips)
      *max_flips = flipsCount;
    *checksum += (permCount % 2 == 0) ? flipsCount : -flipsCount;

    while (1) {
      if (r == n) {
        return;
      }

      int perm0 = perm1[0];
      for (int i = 0; i < r; i++) {
        perm1[i] = perm1[i + 1];
      }
      perm1[r] = perm0;

      count[r]--;
      if (count[r] > 0)
        break;
      r++;
    }
    permCount++;
  }
}

void Fannkuchredux_prepare(Benchmark *self) {
  FannkuchreduxData *data = (FannkuchreduxData *)self->data;
  data->n = Helper_config_i64(self->name, "n");
  if (data->n == 0) {
    data->n = 12;
  }
  data->result_val = 0;
}

void Fannkuchredux_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  FannkuchreduxData *data = (FannkuchreduxData *)self->data;

  int checksum, max_flips;
  fannkuchredux_calculate((int)data->n, &checksum, &max_flips);

  data->result_val =
      (data->result_val + (uint32_t)(checksum * 100 + max_flips)) & 0xFFFFFFFFu;
}

uint32_t Fannkuchredux_checksum(Benchmark *self) {
  FannkuchreduxData *data = (FannkuchreduxData *)self->data;
  return data->result_val;
}

void Fannkuchredux_cleanup(Benchmark *self) { (void)self; }

Benchmark *Fannkuchredux_create(void) {
  Benchmark *bench = Benchmark_create("CLBG::Fannkuchredux");

  FannkuchreduxData *data = malloc(sizeof(FannkuchreduxData));
  data->n = 0;
  data->result_val = 0;

  bench->data = data;
  bench->prepare = Fannkuchredux_prepare;
  bench->run = Fannkuchredux_run;
  bench->checksum = Fannkuchredux_checksum;
  bench->cleanup = Fannkuchredux_cleanup;

  return bench;
}
