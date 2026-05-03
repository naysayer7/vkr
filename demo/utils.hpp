#pragma once

namespace Utils {
std::string FormatMemorySize(size_t bytes) {
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
}  // namespace Utils