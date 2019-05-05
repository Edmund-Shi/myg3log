#include "g3log/common_flags.hpp"
#include <gflags/gflags.h>

DEFINE_string(log_dir, "/tmp/", "the log file path");
DEFINE_int32(v, 0, "show all VLOG(m) messages for m <= this.");
DEFINE_bool(logtostderr, false,
            "log messages go to stderr instead of logfiles");
DEFINE_bool(alsologtostderr, false,
            "log messages go to stderr in addition to logfiles");
