#include "spectralnorm.hpp"
#include <cmath>

Spectralnorm::Spectralnorm() {
  size_val = config_val("size");
  u = std::vector<double>(size_val, 1.0);
  v = std::vector<double>(size_val, 1.0);
}

std::string Spectralnorm::name() const { return "CLBG::Spectralnorm"; }

std::vector<double> Spectralnorm::eval_A_times_u(const std::vector<double> &u) {
  std::vector<double> v(u.size());
  for (size_t i = 0; i < u.size(); i++) {
    double sum = 0.0;
    for (size_t j = 0; j < u.size(); j++) {
      sum += eval_A(static_cast<int>(i), static_cast<int>(j)) * u[j];
    }
    v[i] = sum;
  }
  return v;
}

std::vector<double>
Spectralnorm::eval_At_times_u(const std::vector<double> &u) {
  std::vector<double> v(u.size());
  for (size_t i = 0; i < u.size(); i++) {
    double sum = 0.0;
    for (size_t j = 0; j < u.size(); j++) {
      sum += eval_A(static_cast<int>(j), static_cast<int>(i)) * u[j];
    }
    v[i] = sum;
  }
  return v;
}

std::vector<double>
Spectralnorm::eval_AtA_times_u(const std::vector<double> &u) {
  return eval_At_times_u(eval_A_times_u(u));
}

void Spectralnorm::run(int iteration_id) {
  (void)iteration_id;
  v = eval_AtA_times_u(u);
  u = eval_AtA_times_u(v);
}

uint32_t Spectralnorm::checksum() {
  double vBv = 0.0, vv = 0.0;
  for (int i = 0; i < size_val; i++) {
    vBv += u[i] * v[i];
    vv += v[i] * v[i];
  }
  return Helper::checksum_f64(sqrt(vBv / vv));
}