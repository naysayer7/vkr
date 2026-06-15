#pragma once
#include <string>
#include "tinyfiledialogs.h"

namespace Error {

inline void Handle(const std::exception& exception) {
#ifdef NDEBUG
  tinyfd_messageBox("Ошибка", exception.what(), "ok", "error", 1);
#else
  throw exception;
#endif
}

}  // namespace Error
