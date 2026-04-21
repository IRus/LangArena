#include "benchmark.h"

typedef struct {
  double x, y, z;
} TRVector;

typedef struct {
  TRVector orig, dir;
} TRRay;

typedef struct {
  double r, g, b;
} TRColor;

typedef struct {
  TRVector center;
  double radius;
  TRColor color;
} TRSphere;

typedef struct {
  TRVector position;
  TRColor color;
} TRLight;

typedef struct {
  int32_t width;
  int32_t height;
  uint32_t result_val;
} TextRaytracerData;

static TRVector tr_vector_scale(TRVector v, double s) {
  return (TRVector){v.x * s, v.y * s, v.z * s};
}

static TRVector tr_vector_add(TRVector a, TRVector b) {
  return (TRVector){a.x + b.x, a.y + b.y, a.z + b.z};
}

static TRVector tr_vector_sub(TRVector a, TRVector b) {
  return (TRVector){a.x - b.x, a.y - b.y, a.z - b.z};
}

static double tr_vector_dot(TRVector a, TRVector b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static double tr_vector_magnitude(TRVector v) {
  return sqrt(tr_vector_dot(v, v));
}

static TRVector tr_vector_normalize(TRVector v) {
  double mag = tr_vector_magnitude(v);
  if (mag == 0.0)
    return (TRVector){0, 0, 0};
  return tr_vector_scale(v, 1.0 / mag);
}

static TRColor tr_color_scale(TRColor c, double s) {
  return (TRColor){c.r * s, c.g * s, c.b * s};
}

static TRColor tr_color_add(TRColor a, TRColor b) {
  return (TRColor){a.r + b.r, a.g + b.g, a.b + b.b};
}

static TRVector tr_sphere_get_normal(TRSphere *sphere, TRVector pt) {
  return tr_vector_normalize(tr_vector_sub(pt, sphere->center));
}

static double tr_clamp(double x, double a, double b) {
  if (x < a)
    return a;
  if (x > b)
    return b;
  return x;
}

static double tr_intersect_sphere(TRRay ray, TRVector center, double radius) {
  TRVector l = tr_vector_sub(center, ray.orig);
  double tca = tr_vector_dot(l, ray.dir);
  if (tca < 0.0)
    return -1.0;

  double d2 = tr_vector_dot(l, l) - tca * tca;
  double r2 = radius * radius;
  if (d2 > r2)
    return -1.0;

  double thc = sqrt(r2 - d2);
  double t0 = tca - thc;
  if (t0 > 10000.0)
    return -1.0;

  return t0;
}

static TRColor tr_diffuse_shading(TRVector pi, TRSphere *obj, TRLight light) {
  TRVector n = tr_sphere_get_normal(obj, pi);
  TRVector light_dir = tr_vector_normalize(tr_vector_sub(light.position, pi));
  double lam1 = tr_vector_dot(light_dir, n);
  double lam2 = tr_clamp(lam1, 0.0, 1.0);

  TRColor light_color = tr_color_scale(light.color, lam2 * 0.5);
  TRColor obj_color = tr_color_scale(obj->color, 0.3);
  return tr_color_add(light_color, obj_color);
}

static const char LUT[6] = {'.', '-', '+', '*', 'X', 'M'};

void TextRaytracer_prepare(Benchmark *self) {
  TextRaytracerData *data = (TextRaytracerData *)self->data;

  data->width = (int32_t)Helper_config_i64(self->name, "w");
  data->height = (int32_t)Helper_config_i64(self->name, "h");

  if (data->width <= 0)
    data->width = 10;
  if (data->height <= 0)
    data->height = 10;

  data->result_val = 0;
}

void TextRaytracer_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  TextRaytracerData *data = (TextRaytracerData *)self->data;

  TRSphere scene[3] = {{{-1.0, 0.0, 3.0}, 0.3, {1.0, 0.0, 0.0}},
                       {{0.0, 0.0, 3.0}, 0.8, {0.0, 1.0, 0.0}},
                       {{1.0, 0.0, 3.0}, 0.4, {0.0, 0.0, 1.0}}};

  TRLight light1 = {{0.7, -1.0, 1.7}, {1.0, 1.0, 1.0}};

  uint32_t iteration_result = 0;

  for (int32_t j = 0; j < data->height; j++) {
    for (int32_t i = 0; i < data->width; i++) {
      double fw = data->width;
      double fh = data->height;
      double fi = i;
      double fj = j;

      TRRay ray;
      ray.orig = (TRVector){0.0, 0.0, 0.0};

      TRVector dir = {(fi - fw / 2.0) / fw, (fj - fh / 2.0) / fh, 1.0};
      ray.dir = tr_vector_normalize(dir);

      double tval = -1.0;
      TRSphere *hit_obj = NULL;

      for (int k = 0; k < 3; k++) {
        double intersect =
            tr_intersect_sphere(ray, scene[k].center, scene[k].radius);
        if (intersect >= 0.0) {
          tval = intersect;
          hit_obj = &scene[k];
          break;
        }
      }

      char pixel = ' ';
      if (hit_obj && tval >= 0.0) {
        TRVector pi = tr_vector_add(ray.orig, tr_vector_scale(ray.dir, tval));
        TRColor color = tr_diffuse_shading(pi, hit_obj, light1);
        double col = (color.r + color.g + color.b) / 3.0;
        int idx = (int)(col * 6.0);
        if (idx < 0)
          idx = 0;
        if (idx >= 6)
          idx = 5;
        pixel = LUT[idx];
      }

      iteration_result += (uint8_t)pixel;
    }
  }

  data->result_val += iteration_result;
}

uint32_t TextRaytracer_checksum(Benchmark *self) {
  TextRaytracerData *data = (TextRaytracerData *)self->data;
  return data->result_val;
}

void TextRaytracer_cleanup(Benchmark *self) { (void)self; }

Benchmark *TextRaytracer_create(void) {
  Benchmark *bench = Benchmark_create("Etc::TextRaytracer");

  TextRaytracerData *data = malloc(sizeof(TextRaytracerData));
  memset(data, 0, sizeof(TextRaytracerData));

  bench->data = data;

  bench->prepare = TextRaytracer_prepare;
  bench->run = TextRaytracer_run;
  bench->checksum = TextRaytracer_checksum;
  bench->cleanup = TextRaytracer_cleanup;

  return bench;
}
