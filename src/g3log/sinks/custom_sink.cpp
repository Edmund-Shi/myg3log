#include "g3log/sinks/custom_sink.hpp"

#include "g3log/common_flags.hpp"
#include "g3log/g3log.hpp"
#include "g3log/loglevels.hpp"
#include "g3log/logmessage.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

namespace g3 {

/** Colored log to cout.
 * */

struct CustomSink {
  enum FG_Color { YELLOW = 33, RED = 31, GREEN = 32, WHITE = 97 };
  FG_Color GetColor(const LEVELS level) {
    if (level.value == WARNING.value) {
      return YELLOW;
    }
    if (level.value == DEBUG.value) {
      return GREEN;
    }
    if (g3::internal::wasFatal(level)) {
      return RED;
    }
    return WHITE;
  }

  // custom format function
  std::string ColoredFormatting(const LogMessage &msg) {
    std::string out;
    auto color = GetColor(msg._level);
    out.append(msg.timestamp() + " " + "\033[" + std::to_string(color) + "m" +
               msg.level() + "\033[m" + "[" + msg.file() + "->" +
               msg.function() + ":" + msg.line() + "] " + msg.message());
    return out;
  }

  void PrintMessage(LogMessageMover logEntry) {
    std::cout << ColoredFormatting(logEntry.get()) << std::endl;
  }
};

/** Initialize G3log library like glog.
 *  This will create logworker, parse GFLAGS, add default logger and
 *  call initializeLogging()
 * */
void InitG3Logging(const char *prefix) {
  static auto worker = LogWorker::createLogWorker();
  // worker->addDefaultLogger(prefix, FLAGS_log_dir);
  worker->addSink(std::make_unique<CustomSink>(), &CustomSink::PrintMessage);
  initializeLogging(worker.get());
}
} // namespace g3
