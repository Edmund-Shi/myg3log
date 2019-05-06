#ifndef G3LOG_SINKS_CUSTOMSINK_H_
#define G3LOG_SINKS_CUSTOMSINK_H_
#include "g3log/g3log.hpp"
#include "g3log/loglevels.hpp"
#include "g3log/logworker.hpp"

// add extra logging level
const LEVELS ERROR{WARNING.value + 10, "ERROR"};

namespace g3 {
void InitG3Logging(const char *prefix);
} // namespace g3

#endif
