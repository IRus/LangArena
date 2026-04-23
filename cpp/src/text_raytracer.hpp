#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <optional>
#include <vector>

class TextRaytracer : public Benchmark {
private:
  struct Vector {
    double x, y, z;

    Vector scale(double s) const { return {x * s, y * s, z * s}; }
    Vector add(const Vector &other) const {
      return {x + other.x, y + other.y, z + other.z};
    }
    Vector sub(const Vector &other) const {
      return {x - other.x, y - other.y, z - other.z};
    }
    double dot(const Vector &other) const {
      return x * other.x + y * other.y + z * other.z;
    }
    double magnitude() const { return std::sqrt(dot(*this)); }
    Vector normalize() const;
  };

  struct Ray {
    Vector orig, dir;
  };

  struct Color {
    double r, g, b;

    Color scale(double s) const { return {r * s, g * s, b * s}; }
    Color add(const Color &other) const {
      return {r + other.r, g + other.g, b + other.b};
    }
  };

  struct Sphere {
    Vector center;
    double radius;
    Color color;

    Vector get_normal(const Vector &pt) const {
      return pt.sub(center).normalize();
    }
  };

  struct Light {
    Vector position;
    Color color;
  };

  static constexpr Color WHITE = {1.0, 1.0, 1.0};
  static constexpr Color RED = {1.0, 0.0, 0.0};
  static constexpr Color GREEN = {0.0, 1.0, 0.0};
  static constexpr Color BLUE = {0.0, 0.0, 1.0};

  static constexpr Light LIGHT1 = {{0.7, -1.0, 1.7}, WHITE};
  static constexpr char LUT[6] = {'.', '-', '+', '*', 'X', 'M'};

  std::vector<Sphere> SCENE = {{{-1.0, 0.0, 3.0}, 0.3, RED},
                               {{0.0, 0.0, 3.0}, 0.8, GREEN},
                               {{1.0, 0.0, 3.0}, 0.4, BLUE}};

  int32_t w, h;
  uint32_t result_val;

  int shade_pixel(const Ray &ray, const Sphere &obj, double tval);
  std::optional<double> intersect_sphere(const Ray &ray, const Vector &center,
                                         double radius);
  double clamp(double x, double a, double b);
  Color diffuse_shading(const Vector &pi, const Sphere &obj,
                        const Light &light);

public:
  TextRaytracer();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};