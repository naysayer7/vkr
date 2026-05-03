#pragma once
#include <chrono>
#include <functional>
#include <vector>

namespace Measures {
std::chrono::steady_clock::time_point Start() {
  return std::chrono::steady_clock::now();
}

double ElapsedSeconds(const std::chrono::steady_clock::time_point& start) {
  auto end = std::chrono::steady_clock::now();
  return std::chrono::duration<double>(end - start).count();
}

typedef std::chrono::duration<double, std::nano> Duration;

struct MeasureResult {
  Duration averageTime;
  Duration standardDeviation;
};

Duration RunMeasure(const std::function<void()>& func) {
  const auto start = std::chrono::high_resolution_clock::now();
  func();
  const auto elapsed = std::chrono::high_resolution_clock::now() - start;
  return elapsed;
}

MeasureResult RunMeasure(
    const std::function<void()>& func,
    const size_t iterations,
    const std::function<void(const size_t& iteration,
                             const Duration& time,
                             const Duration& totalTime)>& callback = nullptr,
    const double sigma = 3.0) {
  std::vector<Duration> times(iterations);
  Duration totalTime = Duration(0);
  for (size_t i = 0; i < iterations; i++) {
    times[i] = RunMeasure(func);
    totalTime += times[i];
    if (callback)
      callback(i, times[i], totalTime);
  }

  auto averageTime = totalTime / iterations;
  Duration standardDeviationSquared = Duration(0);
  for (const auto& time : times) {
    standardDeviationSquared +=
        Duration(std::pow((time - averageTime).count(), 2) / iterations);
  }
  Duration standardDeviation =
      Duration(std::sqrt(standardDeviationSquared.count()) * sigma);

  return MeasureResult{averageTime, standardDeviation};
}

}  // namespace Measures