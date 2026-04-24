#include "sieve.hpp"
#include <cmath>
#include <vector>

Sieve::Sieve() : limit(config_val("limit")), checksum_val(0) {}

std::string Sieve::name() const { return "Etc::Sieve"; }

void Sieve::run(int iteration_id) {
  (void)iteration_id;
  size_t sz = static_cast<size_t>(limit);
  std::vector<uint8_t> primes(sz + 1, 1);
  primes[0] = 0;
  primes[1] = 0;

  size_t sqrt_limit =
      static_cast<size_t>(std::sqrt(static_cast<double>(limit)));

  for (size_t p = 2; p <= sqrt_limit; ++p) {
    if (primes[p] == 1) {
      for (size_t multiple = p * p; multiple <= sz; multiple += p) {
        primes[multiple] = 0;
      }
    }
  }

  int last_prime = 2;
  int count = 1;

  for (size_t n = 3; n <= sz; n += 2) {
    if (primes[n] == 1) {
      last_prime = static_cast<int>(n);
      count++;
    }
  }

  checksum_val += static_cast<uint32_t>(last_prime + count);
}

uint32_t Sieve::checksum() { return checksum_val; }