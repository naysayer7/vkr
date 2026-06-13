#pragma once
#include <chrono>
#include <functional>
#include <vector>

namespace Measures {

typedef std::chrono::duration<double, std::nano> Duration;

inline Duration RunMeasure(const std::function<void()>& func) {
  const auto start = std::chrono::high_resolution_clock::now();
  func();
  const auto elapsed = std::chrono::high_resolution_clock::now() - start;
  return elapsed;
}

}  // namespace Measures