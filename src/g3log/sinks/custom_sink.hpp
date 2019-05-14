#pragma once
#include "g3log/g3log.hpp"
#include "g3log/loglevels.hpp"
#include "g3log/logworker.hpp"

// add extra logging level
const LEVELS G3LOG_ERROR{G3LOG_WARNING.value + 10, {"ERROR"}};

namespace g3 {
void InitG3Logging(const char *prefix);
void SetStderrLogging(LEVELS level);
} // namespace g3
