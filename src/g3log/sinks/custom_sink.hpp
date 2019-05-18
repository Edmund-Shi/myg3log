#pragma once
#include "g3log/g3log.hpp"
#include "g3log/loglevels.hpp"
#include "g3log/logworker.hpp"

namespace g3 {
void InitG3Logging(const char *prefix);
void SetStderrLogging(LEVELS level);
} // namespace g3
