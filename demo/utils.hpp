#pragma once
#include <chrono>
#include <format>
#include <string>

namespace Utils {
inline std::string FormatMemorySize(size_t bytes) {
  const char* suffixes[] = {"B", "KB", "MB", "GB"};
  size_t suffixIndex = 0;
  double count = static_cast<double>(bytes);
  while (count >= 1024.0 && suffixIndex < 3) {
    count /= 1024.0;
    ++suffixIndex;
  }
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%.2f %s", count, suffixes[suffixIndex]);
  return std::string(buffer);
}

inline std::string FormatDuration(const Measures::Duration& duration) {
  using namespace std::chrono;
  auto ms = duration_cast<milliseconds>(duration).count();
  auto us = duration_cast<microseconds>(duration).count() % 1000;
  auto ns = duration_cast<nanoseconds>(duration).count() % 1000;
  return std::format("{} ms {} us {} ns", ms, us, ns);
}

inline std::string FormatDuration(const double& duration) {
  return FormatDuration(Measures::Duration(duration));
}

inline int CalculateRunsCount(const int minObjects[2], const int maxObjects[2]) {
  int runs = 0;
  for (int M = maxObjects[0]; M <= maxObjects[1]; ++M)
    for (int m = minObjects[0]; m <= std::min(minObjects[1], (M + 1) / 2); ++m)
      runs++;
  return runs;
}
}  // namespace Utils