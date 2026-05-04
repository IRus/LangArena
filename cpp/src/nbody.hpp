#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <vector>
#include <cmath>

class Nbody : public Benchmark {
private:
  static constexpr double SOLAR_MASS = 4 * M_PI * M_PI;
  static constexpr double DAYS_PER_YEAR = 365.24;

  struct Planet {
    double x, y, z;
    double vx, vy, vz;
    double mass;

    Planet(double x, double y, double z, double vx, double vy, double vz,
           double mass)
        : x(x), y(y), z(z), vx(vx * DAYS_PER_YEAR), vy(vy * DAYS_PER_YEAR),
          vz(vz * DAYS_PER_YEAR), mass(mass * SOLAR_MASS) {}

    void move_from_i(std::vector<Planet> &bodies, int nbodies, double dt,
                     int start);
  };

  std::vector<Planet> bodies;
  double v1;

  double energy();
  void offset_momentum();

public:
  Nbody();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};
