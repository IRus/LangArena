#include "benchmark.h"
#include <yyjson.h>

typedef struct {
  double x, y, z;
  char name[64];
  int opts_val;
  bool opts_bool;
} Coord;

typedef struct {
  Coord *coords;
  int64_t n;
  char *result_str;
  uint32_t result_val;
} JsonGenerateData;

static const char *INFO_STR = "some info";

static double round_to_8_digits(double value) {
  return round(value * 1e8) / 1e8;
}

void JsonGenerate_prepare(Benchmark *self) {
  JsonGenerateData *data = (JsonGenerateData *)self->data;

  if (data->coords) {
    free(data->coords);
    data->coords = NULL;
  }
  if (data->result_str) {
    free(data->result_str);
    data->result_str = NULL;
  }

  data->coords = malloc(data->n * sizeof(Coord));

  for (int64_t i = 0; i < data->n; i++) {
    data->coords[i].x = round_to_8_digits(Helper_next_float(1.0));
    data->coords[i].y = round_to_8_digits(Helper_next_float(1.0));
    data->coords[i].z = round_to_8_digits(Helper_next_float(1.0));

    snprintf(data->coords[i].name, sizeof(data->coords[i].name), "%.7f %u",
             Helper_next_float(1.0), Helper_next_int(10000));

    data->coords[i].opts_val = 1;
    data->coords[i].opts_bool = true;
  }

  data->result_val = 0;
}

void JsonGenerate_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  JsonGenerateData *data = (JsonGenerateData *)self->data;

  if (data->result_str) {
    free(data->result_str);
    data->result_str = NULL;
  }

  yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
  if (!doc)
    return;

  yyjson_mut_val *root = yyjson_mut_obj(doc);
  yyjson_mut_doc_set_root(doc, root);

  yyjson_mut_val *coordinates = yyjson_mut_arr(doc);
  yyjson_mut_obj_add(root, yyjson_mut_str(doc, "coordinates"), coordinates);

  for (int64_t i = 0; i < data->n; i++) {
    Coord *c = &data->coords[i];

    yyjson_mut_val *coord = yyjson_mut_obj(doc);

    yyjson_mut_obj_add(coord, yyjson_mut_str(doc, "x"),
                       yyjson_mut_real(doc, c->x));
    yyjson_mut_obj_add(coord, yyjson_mut_str(doc, "y"),
                       yyjson_mut_real(doc, c->y));
    yyjson_mut_obj_add(coord, yyjson_mut_str(doc, "z"),
                       yyjson_mut_real(doc, c->z));
    yyjson_mut_obj_add(coord, yyjson_mut_str(doc, "name"),
                       yyjson_mut_strcpy(doc, c->name));

    yyjson_mut_val *opts = yyjson_mut_obj(doc);
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    yyjson_mut_arr_add_uint(doc, arr, c->opts_val);
    yyjson_mut_arr_add_bool(doc, arr, c->opts_bool);
    yyjson_mut_obj_add(opts, yyjson_mut_str(doc, "1"), arr);
    yyjson_mut_obj_add(coord, yyjson_mut_str(doc, "opts"), opts);

    yyjson_mut_arr_append(coordinates, coord);
  }

  yyjson_mut_obj_add(root, yyjson_mut_str(doc, "info"),
                     yyjson_mut_str(doc, INFO_STR));

  data->result_str = yyjson_mut_write(doc, 0, NULL);

  if (data->result_str && strlen(data->result_str) >= 15 &&
      strncmp(data->result_str, "{\"coordinates\":", 15) == 0) {
    data->result_val++;
  }

  yyjson_mut_doc_free(doc);
}

uint32_t JsonGenerate_checksum(Benchmark *self) {
  JsonGenerateData *data = (JsonGenerateData *)self->data;
  return data->result_val;
}

void JsonGenerate_cleanup(Benchmark *self) {
  JsonGenerateData *data = (JsonGenerateData *)self->data;

  if (data->coords) {
    free(data->coords);
    data->coords = NULL;
  }
  if (data->result_str) {
    free(data->result_str);
    data->result_str = NULL;
  }
}

Benchmark *JsonGenerate_create(void) {
  Benchmark *bench = Benchmark_create("Json::Generate");

  JsonGenerateData *data = malloc(sizeof(JsonGenerateData));
  memset(data, 0, sizeof(JsonGenerateData));

  bench->data = data;
  data->n = Helper_config_i64(bench->name, "coords");
  bench->prepare = JsonGenerate_prepare;
  bench->run = JsonGenerate_run;
  bench->checksum = JsonGenerate_checksum;
  bench->cleanup = JsonGenerate_cleanup;

  return bench;
}

char *JsonGenerate_get_result(Benchmark *self) {
  JsonGenerateData *data = (JsonGenerateData *)self->data;
  return data->result_str ? strdup(data->result_str) : NULL;
}

typedef struct {
  char *json_text;
  uint32_t result_val;
  int64_t coords_count;
} JsonParseDomData;

static char *generate_json_for_parsing(int64_t coords_count) {
  Benchmark *json_gen_bench = JsonGenerate_create();
  JsonGenerateData *gen_data = (JsonGenerateData *)json_gen_bench->data;

  gen_data->n = coords_count;
  json_gen_bench->prepare(json_gen_bench);
  json_gen_bench->run(json_gen_bench, 0);

  char *json_text = strdup(gen_data->result_str);

  json_gen_bench->cleanup(json_gen_bench);
  free(json_gen_bench->data);
  free(json_gen_bench);

  return json_text;
}

void JsonParseDom_prepare(Benchmark *self) {
  JsonParseDomData *data = (JsonParseDomData *)self->data;

  data->coords_count = Helper_config_i64(self->name, "coords");

  data->json_text = generate_json_for_parsing(data->coords_count);
  data->result_val = 0;
}

void JsonParseDom_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  JsonParseDomData *data = (JsonParseDomData *)self->data;

  yyjson_doc *doc = yyjson_read(data->json_text, strlen(data->json_text), 0);
  if (!doc) {
    return;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root) {
    yyjson_doc_free(doc);
    return;
  }

  yyjson_val *coordinates = yyjson_obj_get(root, "coordinates");
  if (!coordinates || !yyjson_is_arr(coordinates)) {
    yyjson_doc_free(doc);
    return;
  }

  double x_sum = 0.0, y_sum = 0.0, z_sum = 0.0;
  size_t len = 0;

  yyjson_val *coord;
  size_t idx, max;
  yyjson_arr_foreach(coordinates, idx, max, coord) {
    if (!yyjson_is_obj(coord))
      continue;

    yyjson_val *x_val = yyjson_obj_get(coord, "x");
    yyjson_val *y_val = yyjson_obj_get(coord, "y");
    yyjson_val *z_val = yyjson_obj_get(coord, "z");

    if (x_val && y_val && z_val && yyjson_is_num(x_val) &&
        yyjson_is_num(y_val) && yyjson_is_num(z_val)) {

      x_sum += yyjson_get_num(x_val);
      y_sum += yyjson_get_num(y_val);
      z_sum += yyjson_get_num(z_val);
      len++;
    }
  }

  yyjson_doc_free(doc);

  if (len > 0) {
    double x_avg = x_sum / len;
    double y_avg = y_sum / len;
    double z_avg = z_sum / len;

    uint32_t checksum = Helper_checksum_f64(x_avg) +
                        Helper_checksum_f64(y_avg) + Helper_checksum_f64(z_avg);
    data->result_val += checksum;
  }
}

uint32_t JsonParseDom_checksum(Benchmark *self) {
  JsonParseDomData *data = (JsonParseDomData *)self->data;
  return data->result_val;
}

void JsonParseDom_cleanup(Benchmark *self) {
  JsonParseDomData *data = (JsonParseDomData *)self->data;
  if (data->json_text) {
    free(data->json_text);
    data->json_text = NULL;
  }
}

Benchmark *JsonParseDom_create(void) {
  Benchmark *bench = Benchmark_create("Json::ParseDom");

  JsonParseDomData *data = malloc(sizeof(JsonParseDomData));
  memset(data, 0, sizeof(JsonParseDomData));

  bench->data = data;

  bench->prepare = JsonParseDom_prepare;
  bench->run = JsonParseDom_run;
  bench->checksum = JsonParseDom_checksum;
  bench->cleanup = JsonParseDom_cleanup;

  return bench;
}

typedef struct {
  char *json_text;
  uint32_t result_val;
  int64_t coords_count;
} JsonParseMappingData;

void JsonParseMapping_prepare(Benchmark *self) {
  JsonParseMappingData *data = (JsonParseMappingData *)self->data;

  data->coords_count = Helper_config_i64(self->name, "coords");
  if (data->coords_count <= 0) {
    data->coords_count = 1000;
  }

  data->json_text = generate_json_for_parsing(data->coords_count);
  data->result_val = 0;
}

void JsonParseMapping_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  JsonParseMappingData *data = (JsonParseMappingData *)self->data;

  yyjson_doc *doc = yyjson_read(data->json_text, strlen(data->json_text), 0);
  if (!doc) {
    return;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root) {
    yyjson_doc_free(doc);
    return;
  }

  yyjson_val *coordinates = yyjson_obj_get(root, "coordinates");
  if (!coordinates || !yyjson_is_arr(coordinates)) {
    yyjson_doc_free(doc);
    return;
  }

  double x_sum = 0.0, y_sum = 0.0, z_sum = 0.0;
  size_t len = 0;

  yyjson_val *coord;
  size_t idx, max;
  yyjson_arr_foreach(coordinates, idx, max, coord) {
    if (!yyjson_is_obj(coord))
      continue;

    yyjson_val *x_val = yyjson_obj_get(coord, "x");
    yyjson_val *y_val = yyjson_obj_get(coord, "y");
    yyjson_val *z_val = yyjson_obj_get(coord, "z");

    if (x_val && y_val && z_val) {

      x_sum += yyjson_get_num(x_val);
      y_sum += yyjson_get_num(y_val);
      z_sum += yyjson_get_num(z_val);
      len++;
    }
  }

  yyjson_doc_free(doc);

  if (len > 0) {
    double x_avg = x_sum / len;
    double y_avg = y_sum / len;
    double z_avg = z_sum / len;

    uint32_t checksum = Helper_checksum_f64(x_avg) +
                        Helper_checksum_f64(y_avg) + Helper_checksum_f64(z_avg);
    data->result_val += checksum;
  }
}

uint32_t JsonParseMapping_checksum(Benchmark *self) {
  JsonParseMappingData *data = (JsonParseMappingData *)self->data;
  return data->result_val;
}

void JsonParseMapping_cleanup(Benchmark *self) {
  JsonParseMappingData *data = (JsonParseMappingData *)self->data;
  if (data->json_text) {
    free(data->json_text);
    data->json_text = NULL;
  }
}

Benchmark *JsonParseMapping_create(void) {
  Benchmark *bench = Benchmark_create("Json::ParseMapping");

  JsonParseMappingData *data = malloc(sizeof(JsonParseMappingData));
  memset(data, 0, sizeof(JsonParseMappingData));

  bench->data = data;

  bench->prepare = JsonParseMapping_prepare;
  bench->run = JsonParseMapping_run;
  bench->checksum = JsonParseMapping_checksum;
  bench->cleanup = JsonParseMapping_cleanup;

  return bench;
}
