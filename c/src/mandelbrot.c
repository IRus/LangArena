#include "benchmark.h"

typedef struct {
  int64_t w;
  int64_t h;
  uint8_t *result_bin;
  size_t result_size;
  size_t result_capacity;
} MandelbrotData;

static void Mandelbrot_grow_result(MandelbrotData *self, size_t needed) {
  size_t new_capacity = self->result_capacity;
  while (self->result_size + needed >= new_capacity) {
    new_capacity = new_capacity ? new_capacity * 2 : 1024;
  }
  if (new_capacity > self->result_capacity) {
    self->result_bin = realloc(self->result_bin, new_capacity);
    if (!self->result_bin) {
      fprintf(stderr, "Mandelbrot_grow_result: Failed to reallocate memory\n");
      return;
    }
    self->result_capacity = new_capacity;
  }
}

static void Mandelbrot_append(MandelbrotData *self, const uint8_t *data,
                              size_t size) {
  Mandelbrot_grow_result(self, size);
  memcpy(self->result_bin + self->result_size, data, size);
  self->result_size += size;
}

void Mandelbrot_prepare(Benchmark *self) {
  MandelbrotData *data = (MandelbrotData *)self->data;

  data->w = Helper_config_i64(self->name, "w");
  data->h = Helper_config_i64(self->name, "h");

  if (data->w == 0)
    data->w = 200;
  if (data->h == 0)
    data->h = 200;

  data->result_size = 0;
}

void Mandelbrot_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  MandelbrotData *data = (MandelbrotData *)self->data;

  int w = (int)data->w;
  int h = (int)data->h;

  char header[256];
  int header_len = snprintf(header, sizeof(header), "P4\n%d %d\n", w, h);
  Mandelbrot_append(data, (uint8_t *)header, header_len);

  const int ITER = 50;
  const double LIMIT = 2.0;

  int bit_num = 0;
  uint8_t byte_acc = 0;

  for (int y = 0; y < h; y++) {
    double ci = 2.0 * y / (double)h - 1.0;

    for (int x = 0; x < w; x++) {
      double cr = 2.0 * x / (double)w - 1.5;

      double zr = 0.0, zi = 0.0;
      double tr = 0.0, ti = 0.0;

      int i = 0;
      while (i < ITER && tr + ti <= LIMIT * LIMIT) {
        zi = 2.0 * zr * zi + ci;
        zr = tr - ti + cr;
        tr = zr * zr;
        ti = zi * zi;
        i++;
      }

      byte_acc <<= 1;
      if (tr + ti <= LIMIT * LIMIT) {
        byte_acc |= 0x01;
      }
      bit_num++;

      if (bit_num == 8) {
        Mandelbrot_append(data, &byte_acc, 1);
        byte_acc = 0;
        bit_num = 0;
      } else if (x == w - 1) {

        byte_acc <<= (8 - (w % 8));
        Mandelbrot_append(data, &byte_acc, 1);
        byte_acc = 0;
        bit_num = 0;
      }
    }
  }
}

uint32_t Mandelbrot_checksum(Benchmark *self) {
  MandelbrotData *data = (MandelbrotData *)self->data;

  if (!data->result_bin || data->result_size == 0) {
    return 0;
  }

  return Helper_checksum_bytes(data->result_bin, data->result_size);
}

void Mandelbrot_cleanup(Benchmark *self) {
  MandelbrotData *data = (MandelbrotData *)self->data;

  if (data->result_bin) {
    free(data->result_bin);
    data->result_bin = NULL;
  }

  data->result_size = 0;
  data->result_capacity = 0;
}

Benchmark *Mandelbrot_create(void) {
  Benchmark *bench = Benchmark_create("CLBG::Mandelbrot");

  MandelbrotData *data = calloc(1, sizeof(MandelbrotData));

  data->w = Helper_config_i64("CLBG::Mandelbrot", "w");
  data->h = Helper_config_i64("CLBG::Mandelbrot", "h");

  if (data->w == 0)
    data->w = 200;
  if (data->h == 0)
    data->h = 200;

  data->result_bin = NULL;
  data->result_size = 0;
  data->result_capacity = 0;

  bench->data = data;
  bench->prepare = Mandelbrot_prepare;
  bench->run = Mandelbrot_run;
  bench->checksum = Mandelbrot_checksum;
  bench->cleanup = Mandelbrot_cleanup;

  return bench;
}
