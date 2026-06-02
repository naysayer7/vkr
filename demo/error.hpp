#pragma once
#include <string>
#include "tinyfiledialogs.h"

namespace Error {

inline void Show(const std::string& message) {
  tinyfd_messageBox("Ошибка", message.c_str(), "ok", "error", 1);
}

}  // namespace Error
