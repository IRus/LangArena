#include "fannkuchredux.hpp"
#include <algorithm>
#include <numeric>

Fannkuchredux::Fannkuchredux() : n(config_val("n")), result_val(0) {}

std::string Fannkuchredux::name() const { return "CLBG::Fannkuchredux"; }

std::pair<int, int> Fannkuchredux::fannkuchredux(int n) {
  int perm1[32];
  int perm[32];
  int count[32];

  if (n > 32)
    n = 32;

  std::iota(perm1, perm1 + n, 0);

  int maxFlipsCount = 0, permCount = 0, checksum = 0;
  int r = n;

  while (true) {
    while (r > 1) {
      count[r - 1] = r;
      r--;
    }

    std::copy(perm1, perm1 + n, perm);

    int flipsCount = 0;
    int k = perm[0];

    while (k != 0) {
      std::reverse(perm, perm + k + 1);
      flipsCount++;
      k = perm[0];
    }

    maxFlipsCount = std::max(maxFlipsCount, flipsCount);
    checksum += (permCount % 2 == 0) ? flipsCount : -flipsCount;

    while (true) {
      if (r == n)
        return {checksum, maxFlipsCount};

      std::rotate(perm1, perm1 + 1, perm1 + r + 1);

      count[r]--;
      if (count[r] > 0)
        break;
      r++;
    }
    permCount++;
  }
}

void Fannkuchredux::run(int iteration_id) {
  (void)iteration_id;
  auto [a, b] = fannkuchredux(static_cast<int>(n));
  result_val += a * 100 + b;
}

uint32_t Fannkuchredux::checksum() { return result_val; }