#include "benchmark.h"

typedef struct {
  int64_t size_val;
  double *u;
  double *v;
} SpectralnormData;

void Spectralnorm_prepare(Benchmark *self) {
  SpectralnormData *data = (SpectralnormData *)self->data;
  data->size_val = Helper_config_i64(self->name, "size");
  if (data->size_val <= 0) {
    data->size_val = 100;
  }

  data->u = malloc(data->size_val * sizeof(double));
  data->v = malloc(data->size_val * sizeof(double));

  for (int64_t i = 0; i < data->size_val; i++) {
    data->u[i] = 1.0;
    data->v[i] = 1.0;
  }
}

static double eval_A(int64_t i, int64_t j) {
  return 1.0 / ((i + j) * (i + j + 1.0) / 2.0 + i + 1.0);
}

static double *eval_A_times_u(const double *u, int64_t n) {
  double *result = malloc(n * sizeof(double));
  for (int64_t i = 0; i < n; i++) {
    double sum = 0.0;
    for (int64_t j = 0; j < n; j++) {
      sum += eval_A(i, j) * u[j];
    }
    result[i] = sum;
  }
  return result;
}

static double *eval_At_times_u(const double *u, int64_t n) {
  double *result = malloc(n * sizeof(double));
  for (int64_t i = 0; i < n; i++) {
    double sum = 0.0;
    for (int64_t j = 0; j < n; j++) {
      sum += eval_A(j, i) * u[j];
    }
    result[i] = sum;
  }
  return result;
}

static double *eval_AtA_times_u(const double *u, int64_t n) {
  double *temp = eval_A_times_u(u, n);
  double *result = eval_At_times_u(temp, n);
  free(temp);
  return result;
}

void Spectralnorm_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  SpectralnormData *data = (SpectralnormData *)self->data;

  double *new_v = eval_AtA_times_u(data->u, data->size_val);
  double *new_u = eval_AtA_times_u(new_v, data->size_val);

  free(data->u);
  free(data->v);

  data->u = new_u;
  data->v = new_v;
}

uint32_t Spectralnorm_checksum(Benchmark *self) {
  SpectralnormData *data = (SpectralnormData *)self->data;

  double vBv = 0.0, vv = 0.0;
  for (int64_t i = 0; i < data->size_val; i++) {
    vBv += data->u[i] * data->v[i];
    vv += data->v[i] * data->v[i];
  }

  double result = sqrt(vBv / vv);
  return Helper_checksum_f64(result);
}

void Spectralnorm_cleanup(Benchmark *self) {
  SpectralnormData *data = (SpectralnormData *)self->data;
  if (data->u)
    free(data->u);
  if (data->v)
    free(data->v);
}

Benchmark *Spectralnorm_create(void) {
  Benchmark *bench = Benchmark_create("CLBG::Spectralnorm");

  SpectralnormData *data = malloc(sizeof(SpectralnormData));
  memset(data, 0, sizeof(SpectralnormData));

  bench->data = data;

  bench->prepare = Spectralnorm_prepare;
  bench->run = Spectralnorm_run;
  bench->checksum = Spectralnorm_checksum;
  bench->cleanup = Spectralnorm_cleanup;

  return bench;
}
