#include "random.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

void test_chacha() {
  // https://datatracker.ietf.org/doc/html/draft-strombergson-chacha-test-vectors-00
  // Last test case.
  const std::array<std::uint32_t, 8> key = {
    0xb1c16ec4, 0x78a8e88c, 0xe7375a72, 0x35b7df80,
    0x2eed681f, 0xfb794c19, 0xe1beaec6, 0x5d9767a6,
  };
  const std::uint64_t nonce = 0x218268cfd531da1a;
  const std::uint64_t counter = 1;
  const std::array<std::uint32_t, 16> expected_output = {
    0x4ec3fbe5, 0xa9d9a160, 0x5b3417db, 0x3627400a,
    0x10f93b85, 0xf1bd60b0, 0x29b697f8, 0x38d1010f,
    0x904c2cae, 0xeaa95b22, 0xf518d514, 0xa0de2959,
    0x6c7aca98, 0x2712e6cf, 0xe4843c05, 0x32334a9a,
  };

  const auto output = random_private::chacha<20>(key, nonce, counter);
  assert(output == expected_output);
}

void test_uniform_int() {
  Random random("foo", 123);

  for (const int n : {17, 1900000000}) {
    const double n_d = n;
    const double mean = (n_d - 1.0) / 2.0;
    const double variance = (n_d * n_d - 1.0) / 12.0;
    const int num_iters = 1000000;
    double total = 0.0;
    for (int i=0; i < num_iters; ++i) {
      const int a = random.uniform_int(0, n - 1);
      assert(a >= 0 && a < n);
      total += a;
    }

    assert(std::fabs(total - num_iters * mean) < 4.0 * std::sqrt(num_iters * variance));
  }
}

void test_shuffle() {
  Random random("foo", 123);
  const std::array<int, 3> v = {1,2,3};
  std::array<int, 3> w = v;
  random.shuffle(w.begin(), w.end());
  std::sort(w.begin(), w.end());
  assert(w == v);
}

int main() {
    test_chacha();
    test_uniform_int();
    test_shuffle();
    std::cout << "OK\n";
}
