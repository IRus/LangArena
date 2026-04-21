#include "benchmark.h"
#include <pthread.h>

static double **matgen(int n) {
  double tmp = 1.0 / n / n;
  double **a = malloc(n * sizeof(double *));

  for (int i = 0; i < n; i++) {
    a[i] = malloc(n * sizeof(double));
    for (int j = 0; j < n; j++) {
      a[i][j] = tmp * (i - j) * (i + j);
    }
  }
  return a;
}

static void free_matrix(double **a, int n) {
  if (!a)
    return;
  for (int i = 0; i < n; i++) {
    free(a[i]);
  }
  free(a);
}

static double **transpose(double **b, int n) {
  double **b_t = malloc(n * sizeof(double *));
  for (int j = 0; j < n; j++) {
    b_t[j] = malloc(n * sizeof(double));
    for (int i = 0; i < n; i++) {
      b_t[j][i] = b[i][j];
    }
  }
  return b_t;
}

typedef struct {
  int64_t n;
  uint32_t result_val;
  double **a;
  double **b;
} MatmulBaseData;

static uint32_t Matmul_checksum(Benchmark *self) {
  MatmulBaseData *data = (MatmulBaseData *)self->data;
  return data->result_val;
}

static void MatmulBase_prepare(Benchmark *self) {
  MatmulBaseData *data = (MatmulBaseData *)self->data;
  int n = (int)data->n;

  data->a = matgen(n);
  data->b = matgen(n);
  data->result_val = 0;
}

static void MatmulBase_cleanup(Benchmark *self) {
  MatmulBaseData *data = (MatmulBaseData *)self->data;
  int n = (int)data->n;

  if (data->a) {
    free_matrix(data->a, n);
    data->a = NULL;
  }
  if (data->b) {
    free_matrix(data->b, n);
    data->b = NULL;
  }
}

static double **matmul_sequential(double **a, double **b, int n) {
  double **b_t = transpose(b, n);
  double **c = malloc(n * sizeof(double *));

  for (int i = 0; i < n; i++) {
    c[i] = malloc(n * sizeof(double));
    double *ai = a[i];
    for (int j = 0; j < n; j++) {
      double s = 0.0;
      double *b_tj = b_t[j];

      for (int k = 0; k < n; k++) {
        s += ai[k] * b_tj[k];
      }
      c[i][j] = s;
    }
  }

  free_matrix(b_t, n);
  return c;
}

static void Matmul_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  MatmulBaseData *data = (MatmulBaseData *)self->data;
  int n = (int)data->n;

  double **c = matmul_sequential(data->a, data->b, n);
  double center_value = c[n >> 1][n >> 1];
  free_matrix(c, n);

  uint32_t iter_checksum = Helper_checksum_f64(center_value);
  data->result_val += iter_checksum;
}

Benchmark *Matmul_create(void) {
  Benchmark *bench = Benchmark_create("Matmul::Single");
  MatmulBaseData *data = malloc(sizeof(MatmulBaseData));

  data->n = Helper_config_i64("Matmul::Single", "n");
  if (data->n == 0)
    data->n = 100;
  data->result_val = 0;
  data->a = NULL;
  data->b = NULL;

  bench->data = data;
  bench->prepare = MatmulBase_prepare;
  bench->run = Matmul_run;
  bench->checksum = Matmul_checksum;
  bench->cleanup = MatmulBase_cleanup;

  return bench;
}

typedef struct {
  MatmulBaseData base;
  int num_threads;
} MatmulParallelData;

typedef struct {
  double **a;
  double **b_t;
  double **c;
  int n;
  int start_row;
  int end_row;
} MatmulThreadData;

static void *thread_func(void *arg) {
  MatmulThreadData *data = (MatmulThreadData *)arg;

  for (int i = data->start_row; i < data->end_row; i++) {
    double *ai = data->a[i];
    double *ci = data->c[i];

    for (int j = 0; j < data->n; j++) {
      double sum = 0.0;
      double *b_tj = data->b_t[j];

      for (int k = 0; k < data->n; k++) {
        sum += ai[k] * b_tj[k];
      }
      ci[j] = sum;
    }
  }
  return NULL;
}

static double **matmul_parallel(double **a, double **b, int n,
                                int num_threads) {
  pthread_t threads[num_threads];
  MatmulThreadData thread_data[num_threads];

  double **b_t = transpose(b, n);
  double **c = malloc(n * sizeof(double *));
  for (int i = 0; i < n; i++) {
    c[i] = calloc(n, sizeof(double));
  }

  int rows_per_thread = (n + num_threads - 1) / num_threads;

  for (int t = 0; t < num_threads; t++) {
    thread_data[t].a = a;
    thread_data[t].b_t = b_t;
    thread_data[t].c = c;
    thread_data[t].n = n;
    thread_data[t].start_row = t * rows_per_thread;
    thread_data[t].end_row = thread_data[t].start_row + rows_per_thread;
    if (thread_data[t].end_row > n || t == num_threads - 1) {
      thread_data[t].end_row = n;
    }

    int rc = pthread_create(&threads[t], NULL, thread_func, &thread_data[t]);
    if (rc != 0) {
      fprintf(stderr, "Failed to create thread %d\n", t);
      thread_func(&thread_data[t]);
      threads[t] = 0;
    }
  }

  for (int t = 0; t < num_threads; t++) {
    if (threads[t] != 0) {
      pthread_join(threads[t], NULL);
    }
  }

  free_matrix(b_t, n);
  return c;
}

static void MatmulParallel_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  MatmulParallelData *data = (MatmulParallelData *)self->data;
  int n = (int)data->base.n;

  double **c =
      matmul_parallel(data->base.a, data->base.b, n, data->num_threads);
  double center_value = c[n >> 1][n >> 1];
  free_matrix(c, n);

  uint32_t iter_checksum = Helper_checksum_f64(center_value);
  data->base.result_val += iter_checksum;
}

Benchmark *Matmul4T_create(void) {
  Benchmark *bench = Benchmark_create("Matmul::T4");
  MatmulParallelData *data = malloc(sizeof(MatmulParallelData));

  data->base.n = Helper_config_i64("Matmul::T4", "n");
  if (data->base.n == 0)
    data->base.n = 100;
  data->base.result_val = 0;
  data->base.a = NULL;
  data->base.b = NULL;
  data->num_threads = 4;

  bench->data = data;
  bench->prepare = MatmulBase_prepare;
  bench->run = MatmulParallel_run;
  bench->checksum = Matmul_checksum;
  bench->cleanup = MatmulBase_cleanup;

  return bench;
}

Benchmark *Matmul8T_create(void) {
  Benchmark *bench = Benchmark_create("Matmul::T8");
  MatmulParallelData *data = malloc(sizeof(MatmulParallelData));

  data->base.n = Helper_config_i64("Matmul::T8", "n");
  if (data->base.n == 0)
    data->base.n = 100;
  data->base.result_val = 0;
  data->base.a = NULL;
  data->base.b = NULL;
  data->num_threads = 8;

  bench->data = data;
  bench->prepare = MatmulBase_prepare;
  bench->run = MatmulParallel_run;
  bench->checksum = Matmul_checksum;
  bench->cleanup = MatmulBase_cleanup;

  return bench;
}

Benchmark *Matmul16T_create(void) {
  Benchmark *bench = Benchmark_create("Matmul::T16");
  MatmulParallelData *data = malloc(sizeof(MatmulParallelData));

  data->base.n = Helper_config_i64("Matmul::T16", "n");
  if (data->base.n == 0)
    data->base.n = 100;
  data->base.result_val = 0;
  data->base.a = NULL;
  data->base.b = NULL;
  data->num_threads = 16;

  bench->data = data;
  bench->prepare = MatmulBase_prepare;
  bench->run = MatmulParallel_run;
  bench->checksum = Matmul_checksum;
  bench->cleanup = MatmulBase_cleanup;

  return bench;
}
