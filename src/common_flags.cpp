#include "g3log/common_flags.hpp"
#include <gflags/gflags.h>

DEFINE_string(log_dir, "/tmp/", "the log file path");
DEFINE_int32(v, 0, "show all VLOG(m) messages for m <= this.");
DEFINE_int32(stderrthreshold, 2,
             "log messages at or above this level are copied to stderr in "
             "addition to logfiles. This flag obsoletes --alsologtostderr. 0 "
             "for INFO/DEBUG, 1 for WARNNING, 2 for ERROR and 3 for FATAL");
DEFINE_bool(logtostderr, false,
            "log messages go to stderr instead of logfiles");
DEFINE_bool(alsologtostderr, false,
            "log messages go to stderr in addition to logfiles");
DEFINE_bool(colorlogtostderr, true,
            "always log with color, to be compatible with glog");
DEFINE_string(log_link, "",
              "Put additional links to the lates "
              "log files in this directory");
