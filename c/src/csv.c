#include "benchmark.h"

typedef struct {
  char *csv_data;
  uint32_t result_val;
  int64_t rows;
} CsvParseData;

typedef struct {
  double x, y, z;
} Point;

static char *generate_csv_for_parsing(int64_t rows) {
  size_t capacity = rows * 100 + 1;
  char *buffer = malloc(capacity);
  if (!buffer)
    return NULL;

  size_t pos = 0;
  for (int64_t i = 0; i < rows; i++) {
    char c = 'A' + (i % 26);
    double x = Helper_next_float(1.0);
    double z = Helper_next_float(1.0);
    double y = Helper_next_float(1.0);
    pos += snprintf(
        buffer + pos, capacity - pos,
        "\"point %c\\n, \"\"%lld\"\"\",%.10f,,%.10f,\"[%s\\n, %lld]\",%.10f\n",
        c, (long long)(i % 100), x, z, (i % 2 == 0) ? "true" : "false",
        (long long)(i % 100), y);
  }
  return buffer;
}

void CsvParse_prepare(Benchmark *self) {
  CsvParseData *data = (CsvParseData *)self->data;
  data->rows = Helper_config_i64(self->name, "rows");
  data->csv_data = generate_csv_for_parsing(data->rows);
  data->result_val = 0;
}

static int parse_csv_line(const char *line, double *x, double *y, double *z) {
  int field_idx = 0;

  const char *start = line;
  int in_quotes = 0;

  const char *field_starts[6] = {0};
  const char *field_ends[6] = {0};

  for (const char *p = line; *p && field_idx < 6; p++) {
    if (*p == '"') {
      in_quotes = !in_quotes;
    } else if (*p == ',' && !in_quotes) {
      field_starts[field_idx] = start;
      field_ends[field_idx] = p;
      field_idx++;
      start = p + 1;
    }
  }

  if (field_idx < 6 && *start) {
    field_starts[field_idx] = start;
    field_ends[field_idx] = start + strlen(start);
    field_idx++;
  }

  if (field_idx < 6)
    return -1;

  char buf[64];
  size_t len = field_ends[1] - field_starts[1];
  if (len >= sizeof(buf))
    return -1;
  memcpy(buf, field_starts[1], len);
  buf[len] = '\0';
  *x = strtod(buf, NULL);

  len = field_ends[3] - field_starts[3];
  if (len >= sizeof(buf))
    return -1;
  memcpy(buf, field_starts[3], len);
  buf[len] = '\0';
  *z = strtod(buf, NULL);

  len = field_ends[5] - field_starts[5];
  if (len >= sizeof(buf))
    return -1;
  memcpy(buf, field_starts[5], len);
  buf[len] = '\0';
  *y = strtod(buf, NULL);

  return 0;
}

void CsvParse_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  CsvParseData *data = (CsvParseData *)self->data;
  if (!data->csv_data)
    return;

  Point *points = malloc(data->rows * sizeof(Point));
  if (!points)
    return;

  int point_count = 0;
  const char *s = data->csv_data;

  while (*s && point_count < data->rows) {
    const char *e = strchr(s, '\n');
    if (!e)
      e = s + strlen(s);

    char line[512];
    size_t len = e - s;
    if (len < sizeof(line)) {
      memcpy(line, s, len);
      line[len] = '\0';

      double x, y, z;
      if (parse_csv_line(line, &x, &y, &z) == 0) {
        points[point_count].x = x;
        points[point_count].y = y;
        points[point_count].z = z;
        point_count++;
      }
    }
    s = e + 1;
  }

  if (point_count > 0) {
    double x_sum = 0, y_sum = 0, z_sum = 0;
    for (int i = 0; i < point_count; i++) {
      x_sum += points[i].x;
      y_sum += points[i].y;
      z_sum += points[i].z;
    }

    double len = point_count;
    data->result_val += Helper_checksum_f64(x_sum / len) +
                        Helper_checksum_f64(y_sum / len) +
                        Helper_checksum_f64(z_sum / len);
  }

  free(points);
}

uint32_t CsvParse_checksum(Benchmark *self) {
  CsvParseData *data = (CsvParseData *)self->data;
  return data->result_val;
}

void CsvParse_cleanup(Benchmark *self) {
  CsvParseData *data = (CsvParseData *)self->data;
  if (data->csv_data) {
    free(data->csv_data);
    data->csv_data = NULL;
  }
}

Benchmark *CsvParse_create(void) {
  Benchmark *bench = Benchmark_create("CSV::Parse");
  if (!bench)
    return NULL;

  CsvParseData *data = calloc(1, sizeof(CsvParseData));
  if (!data) {
    free(bench);
    return NULL;
  }

  bench->data = data;
  bench->prepare = CsvParse_prepare;
  bench->run = CsvParse_run;
  bench->checksum = CsvParse_checksum;
  bench->cleanup = CsvParse_cleanup;

  return bench;
}
