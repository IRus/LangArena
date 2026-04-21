#include "benchmark.h"

typedef struct {
  int64_t limit;
  uint32_t checksum;
} SieveData;

static int *sieve_generate(int64_t limit, int *count) {
  if (limit < 2) {
    *count = 0;
    return NULL;
  }

  uint8_t *primes = malloc((limit + 1) * sizeof(uint8_t));
  if (!primes)
    return NULL;

  memset(primes, 1, (limit + 1) * sizeof(uint8_t));
  primes[0] = 0;
  primes[1] = 0;

  int sqrt_limit = (int)sqrt((double)limit);

  for (int p = 2; p <= sqrt_limit; p++) {
    if (primes[p] == 1) {

      for (int multiple = p * p; multiple <= limit; multiple += p) {
        primes[multiple] = 0;
      }
    }
  }

  int last_prime = 2;
  int count_primes = 1;

  for (int n = 3; n <= limit; n += 2) {
    if (primes[n] == 1) {
      last_prime = n;
      count_primes++;
    }
  }

  free(primes);

  *count = last_prime + count_primes;
  return NULL;
}

void Sieve_prepare(Benchmark *self) {
  SieveData *data = (SieveData *)self->data;
  data->limit = Helper_config_i64(self->name, "limit");
  if (data->limit <= 0)
    data->limit = 1000000;
  data->checksum = 0;
}

void Sieve_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  SieveData *data = (SieveData *)self->data;

  int sum = 0;
  sieve_generate(data->limit, &sum);

  data->checksum += (uint32_t)sum;
}

uint32_t Sieve_checksum(Benchmark *self) {
  SieveData *data = (SieveData *)self->data;
  return data->checksum;
}

Benchmark *Sieve_create(void) {
  Benchmark *bench = Benchmark_create("Etc::Sieve");

  SieveData *data = malloc(sizeof(SieveData));
  memset(data, 0, sizeof(SieveData));

  bench->data = data;
  bench->prepare = Sieve_prepare;
  bench->run = Sieve_run;
  bench->checksum = Sieve_checksum;

  return bench;
}
