#include "text_raytracer.hpp"
#include <cmath>

TextRaytracer::Vector TextRaytracer::Vector::normalize() const {
  double mag = magnitude();
  if (mag == 0.0)
    return {0, 0, 0};
  return scale(1.0 / mag);
}

TextRaytracer::TextRaytracer() : result_val(0) {
  w = static_cast<int32_t>(config_val("w"));
  h = static_cast<int32_t>(config_val("h"));
}

std::string TextRaytracer::name() const { return "Etc::TextRaytracer"; }

int TextRaytracer::shade_pixel(const Ray &ray, const Sphere &obj, double tval) {
  Vector pi = ray.orig.add(ray.dir.scale(tval));
  Color color = diffuse_shading(pi, obj, LIGHT1);
  double col = (color.r + color.g + color.b) / 3.0;
  int idx = static_cast<int>(col * 6.0);
  if (idx < 0)
    idx = 0;
  if (idx >= 6)
    idx = 5;
  return idx;
}

std::optional<double> TextRaytracer::intersect_sphere(const Ray &ray,
                                                      const Vector &center,
                                                      double radius) {
  Vector l = center.sub(ray.orig);
  double tca = l.dot(ray.dir);
  if (tca < 0.0)
    return std::nullopt;

  double d2 = l.dot(l) - tca * tca;
  double r2 = radius * radius;
  if (d2 > r2)
    return std::nullopt;

  double thc = std::sqrt(r2 - d2);
  double t0 = tca - thc;
  if (t0 > 10000.0)
    return std::nullopt;

  return t0;
}

double TextRaytracer::clamp(double x, double a, double b) {
  if (x < a)
    return a;
  if (x > b)
    return b;
  return x;
}

TextRaytracer::Color TextRaytracer::diffuse_shading(const Vector &pi,
                                                    const Sphere &obj,
                                                    const Light &light) {
  Vector n = obj.get_normal(pi);
  Vector light_dir = light.position.sub(pi).normalize();
  double lam1 = light_dir.dot(n);
  double lam2 = clamp(lam1, 0.0, 1.0);
  return light.color.scale(lam2 * 0.5).add(obj.color.scale(0.3));
}

void TextRaytracer::run(int iteration_id) {
  (void)iteration_id;
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      double fw = w, fi = i, fj = j, fh = h;

      Ray ray{
          {0.0, 0.0, 0.0},
          Vector{(fi - fw / 2.0) / fw, (fj - fh / 2.0) / fh, 1.0}.normalize()};

      std::optional<double> tval;
      const Sphere *hit_obj = nullptr;

      for (const auto &obj : SCENE) {
        auto intersect = intersect_sphere(ray, obj.center, obj.radius);
        if (intersect) {
          tval = intersect;
          hit_obj = &obj;
          break;
        }
      }

      char pixel = ' ';
      if (hit_obj && tval) {
        pixel = LUT[shade_pixel(ray, *hit_obj, *tval)];
      }
      result_val += static_cast<uint8_t>(pixel);
    }
  }
}

uint32_t TextRaytracer::checksum() { return result_val; }