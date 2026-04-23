#include "matmul.hpp"
#include <algorithm>
#include <thread>

Matmul1T::Matmul1T() : result_val(0) {}

std::string Matmul1T::name() const { return "Matmul::Single"; }

std::vector<std::vector<double>> Matmul1T::matgen(int n) {
  double tmp = 1.0 / n / n;
  std::vector<std::vector<double>> a(n, std::vector<double>(n));
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      a[i][j] = tmp * (i - j) * (i + j);
    }
  }
  return a;
}

std::vector<std::vector<double>>
Matmul1T::matmul(int n, const std::vector<std::vector<double>> &a,
                 const std::vector<std::vector<double>> &b) {

  std::vector<std::vector<double>> b_t(n, std::vector<double>(n));
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      b_t[j][i] = b[i][j];
    }
  }

  std::vector<std::vector<double>> c(n, std::vector<double>(n));
  for (int i = 0; i < n; i++) {
    const auto &ai = a[i];
    auto &ci = c[i];
    for (int j = 0; j < n; j++) {
      double s = 0.0;
      const auto &b_tj = b_t[j];
      for (int k = 0; k < n; k++) {
        s += ai[k] * b_tj[k];
      }
      ci[j] = s;
    }
  }
  return c;
}

void Matmul1T::prepare() {
  int n = static_cast<int>(config_val("n"));
  a = matgen(n);
  b = matgen(n);
}

void Matmul1T::run(int) {
  int n = static_cast<int>(a.size());
  auto c = matmul(n, a, b);
  result_val += Helper::checksum_f64(c[n >> 1][n >> 1]);
}

uint32_t Matmul1T::checksum() { return result_val; }

std::string Matmul4T::name() const { return "Matmul::T4"; }

std::vector<std::vector<double>>
Matmul4T::matmul_parallel(int n, const std::vector<std::vector<double>> &a,
                          const std::vector<std::vector<double>> &b) {
  int num_threads = get_num_threads();

  std::vector<std::vector<double>> b_t(n, std::vector<double>(n));
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      b_t[j][i] = b[i][j];
    }
  }

  std::vector<std::vector<double>> c(n, std::vector<double>(n));
  std::vector<std::thread> threads;
  threads.reserve(num_threads);

  int rows_per_thread = (n + num_threads - 1) / num_threads;

  for (int t = 0; t < num_threads; t++) {
    int start = t * rows_per_thread;
    int end = std::min(start + rows_per_thread, n);

    threads.emplace_back([&, start, end]() {
      for (int i = start; i < end; i++) {
        const auto &ai = a[i];
        auto &ci = c[i];
        for (int j = 0; j < n; j++) {
          double sum = 0.0;
          const auto &b_tj = b_t[j];
          for (int k = 0; k < n; k++) {
            sum += ai[k] * b_tj[k];
          }
          ci[j] = sum;
        }
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }
  return c;
}

void Matmul4T::run(int) {
  int n = static_cast<int>(a.size());
  auto c = matmul_parallel(n, a, b);
  result_val += Helper::checksum_f64(c[n >> 1][n >> 1]);
}

std::string Matmul8T::name() const { return "Matmul::T8"; }

std::string Matmul16T::name() const { return "Matmul::T16"; }