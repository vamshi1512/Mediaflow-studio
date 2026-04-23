#pragma once

#include <algorithm>
#include <cstdint>
#include <random>

namespace mediaflow {

class DeterministicRng {
 public:
  explicit DeterministicRng(std::uint64_t seed = 1) : engine_(seed) {}

  void reseed(std::uint64_t seed) { engine_.seed(seed); }

  double uniform(double min, double max) {
    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(engine_);
  }

  int uniform_int(int min, int max) {
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(engine_);
  }

  bool bernoulli(double probability) {
    const double bounded_probability = std::clamp(probability, 0.0, 1.0);
    std::bernoulli_distribution distribution(bounded_probability);
    return distribution(engine_);
  }

 private:
  std::mt19937_64 engine_;
};

}  // namespace mediaflow
