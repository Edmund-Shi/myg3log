#include "g3log/sinks/custom_sink.hpp"

#include "g3log/common_flags.hpp"
#include "g3log/g3log.hpp"
#include "g3log/loglevels.hpp"
#include "g3log/logmessage.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

namespace g3 {
namespace {
int g_stderrthreshold = 0;
}

/** Colored log to cout.
 * */

struct CustomSink {
  CustomSink() {
    // turn off stdio sync
    std::ios::sync_with_stdio(false);
  }
  enum FG_Color { YELLOW = 33, RED = 31, GREEN = 32, WHITE = 97 };
  FG_Color GetColor(const LEVELS level) {
    if (level.value == G3LOG_WARNING.value) {
      return YELLOW;
    }
    if (level.value == G3LOG_DEBUG.value) {
      return GREEN;
    }
    if (level.value == G3LOG_ERROR.value) {
      return RED;
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
    // highligt whole line
    out.append("\033[" + std::to_string(color) + "m" + msg.shortLevel() +
               msg.timestamp() + " " + msg.file() + "->" + msg.function() +
               ":" + msg.line() + "] " + msg.message() + "\033[m");
    return out;
  }

  void PrintMessage(LogMessageMover logEntry) {
    if (logEntry.get().level_value() >= g_stderrthreshold)
      std::clog << ColoredFormatting(logEntry.get()) << std::endl;
  }
};

/** Initialize G3log library like glog.
 *  This will create logworker, parse GFLAGS, add default logger and
 *  call initializeLogging()
 * */
void InitG3Logging(const char *prefix) {
  // add custom log level
  only_change_at_initialization::addLogLevel(G3LOG_ERROR);
  static auto worker = LogWorker::createLogWorker();

  // determine stderr threshold
  switch (FLAGS_stderrthreshold) {
  case 0:
    // INFO and DEBUG share same level
    // DEBUG is used for coloring
    g_stderrthreshold = G3LOG_DEBUG.value;
    break;
  case 1:
    g_stderrthreshold = G3LOG_WARNING.value;
    break;
  case 2:
    g_stderrthreshold = G3LOG_ERROR.value;
    break;
  case 3:
    g_stderrthreshold = G3LOG_FATAL.value;
    break;
  default:
    g_stderrthreshold = G3LOG_ERROR.value;
    break;
  }

  if (FLAGS_logtostderr) {
    std::clog << "Only log to stderr" << std::endl;
    // log all to stderr
    g_stderrthreshold = G3LOG_DEBUG.value;
    worker->addSink(std::make_unique<CustomSink>(), &CustomSink::PrintMessage);
  } else if (FLAGS_alsologtostderr) {
    g_stderrthreshold = G3LOG_DEBUG.value;
    worker->addSink(std::make_unique<CustomSink>(), &CustomSink::PrintMessage);
    worker->addDefaultLogger(prefix, FLAGS_log_dir);
  } else {
    // log to file in addition to logmessage above threshold
    worker->addSink(std::make_unique<CustomSink>(), &CustomSink::PrintMessage);
    worker->addDefaultLogger(prefix, FLAGS_log_dir);
  }
  initializeLogging(worker.get());
}
void SetStderrLogging(LEVELS level) { g_stderrthreshold = level.value; }
} // namespace g3
