/** ==========================================================================
 * 2011 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
 * with no warranties. This code is yours to share, use and modify with no
 * strings attached and no restrictions or obligations.
 *
 * For more information see g3log/LICENSE or refer refer to http://unlicense.org
 * ============================================================================
 *
 * Filename:g3log.hpp  Framework for Logging and Design By Contract
 * Created: 2011 by Kjell Hedström
 *
 * PUBLIC DOMAIN and Not copywrited since it was built on public-domain software
 * and influenced at least in "spirit" from the following sources
 * 1. kjellkod.cc ;)
 * 2. Dr.Dobbs, Petru Marginean:
 * http://drdobbs.com/article/printableArticle.jhtml?articleId=201804215&dept_url=/caddpp/
 * 3. Dr.Dobbs, Michael Schulze:
 * http://drdobbs.com/article/printableArticle.jhtml?articleId=225700666&dept_url=/cpp/
 * 4. Google 'glog': http://google-glog.googlecode.com/svn/trunk/doc/glog.html
 * 5. Various Q&A at StackOverflow
 * ********************************************* */

#pragma once

#include "g3log/common_flags.hpp"
#include "g3log/generated_definitions.hpp"
#include "g3log/logcapture.hpp"
#include "g3log/loglevels.hpp"
#include "g3log/logmessage.hpp"

#include <functional>
#include <string>

#if !(defined(__PRETTY_FUNCTION__))
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

// thread_local doesn't exist before VS2013
// it exists on VS2015
#if !(defined(thread_local)) && defined(_MSC_VER) && _MSC_VER < 1900
#define thread_local __declspec(thread)
#endif

/** namespace for GLOG_LOG() and CHECK() frameworks
 * History lesson:   Why the names 'g3' and 'g3log'?:
 * The framework was made in my own free time as PUBLIC DOMAIN but the
 * first commercial project to use it used 'g3' as an internal denominator for
 * the current project. g3 as in 'generation 2'. I decided to keep the g3 and
 * g3log names to give credit to the people in that project (you know who you
 * are :) and I guess also for 'sentimental' reasons. That a big influence was
 * Google's glog is just a happy coincidence or subconscious choice. Either way
 * g3log became the name for this logger.
 *
 * --- Thanks for a great 2011 and good luck with 'g3' --- KjellKod
 */
namespace g3 {
class LogWorker;
struct LogMessage;
struct FatalMessage;

/** Should be called at very first startup of the software with \ref g3LogWorker
 *  pointer. Ownership of the \ref g3LogWorker is the responsibility of the
 * caller */
void initializeLogging(LogWorker *logger);

/** setFatalPreLoggingHook() provides an optional extra step before the
 * fatalExitHandler is called
 *
 * Set a function-hook before a fatal message will be sent to the logger
 * i.e. this is a great place to put a break point, either in your debugger
 * or programatically to catch GLOG_LOG(FATAL), CHECK(...) or an OS fatal event
 * (exception or signal) This will be reset to default (does nothing) at
 * initializeLogging(...);
 *
 * Example usage:
 * Windows: g3::setFatalPreLoggingHook([]{__debugbreak();}); // remember
 * #include <intrin.h> WARNING: '__debugbreak()' when not running in Debug in
 * your Visual Studio IDE will likely trigger a recursive crash if used here. It
 * should only be used when debugging in your Visual Studio IDE. Recursive
 * crashes are handled but are unnecessary.
 *
 * Linux:   g3::setFatalPreLoggingHook([]{ raise(SIGTRAP); });
 */
void setFatalPreLoggingHook(std::function<void(void)> pre_fatal_hook);

/** If the @ref setFatalPreLoggingHook is not enough and full fatal exit
 * handling is needed then use "setFatalExithandler".  Please see g3log.cpp and
 * crashhandler_windows.cpp or crashhandler_unix for example of restoring signal
 * and exception handlers, flushing the log and shutting down.
 */
void setFatalExitHandler(std::function<void(FatalMessagePtr)> fatal_call);

#ifdef G3_DYNAMIC_MAX_MESSAGE_SIZE
// only_change_at_initialization namespace is for changes to be done only during
// initialization. More specifically items here would be called prior to calling
// other parts of g3log
namespace only_change_at_initialization {
// Sets the MaxMessageSize to be used when capturing log messages. Currently
// this value is set to 2KB. Messages Longer than this are bound to 2KB with the
// string "[...truncated...]" at the end. This function allows this limit to be
// changed.
void setMaxMessageSize(size_t max_size);
} // namespace only_change_at_initialization
#endif /* G3_DYNAMIC_MAX_MESSAGE_SIZE */

// internal namespace is for completely internal or semi-hidden from the g3
// namespace due to that it is unlikely that you will use these
namespace internal {
/// @returns true if logger is initialized
bool isLoggingInitialized();

// Save the created LogMessage to any existing sinks
void saveMessage(const char *message, const char *file, int line,
                 const char *function, const LEVELS &level,
                 const char *boolean_expression, int fatal_signal,
                 const char *stack_trace);

// forwards the message to all sinks
void pushMessageToLogger(LogMessagePtr log_entry);

// forwards a FATAL message to all sinks,. after which the g3logworker
// will trigger crashhandler / g3::internal::exitWithDefaultSignalHandler
//
// By default the "fatalCall" will forward a FatalMessageptr to this function
// this behavior can be changed if you set a different fatal handler through
// "setFatalExitHandler"
void pushFatalMessageToLogger(FatalMessagePtr message);

// Saves the created FatalMessage to any existing sinks and exits with
// the originating fatal signal,. or SIGABRT if it originated from a broken
// contract. By default forwards to: pushFatalMessageToLogger, see
// "setFatalExitHandler" to override
//
// If you override it then you probably want to call "pushFatalMessageToLogger"
// after your custom fatal handler is done. This will make sure that the fatal
// message the pushed to sinks as well as shutting down the process
void fatalCall(FatalMessagePtr message);

// Shuts down logging. No object cleanup but further GLOG_LOG(...) calls will be
// ignored.
void shutDownLogging();

// Shutdown logging, but ONLY if the active logger corresponds to the one
// currently initialized
bool shutDownLoggingForActiveOnly(LogWorker *active);

} // namespace internal
} // namespace g3

#define INTERNAL_LOG_MESSAGE(level)                                            \
  LogCapture(__FILE__, __LINE__,                                               \
             static_cast<const char *>(__PRETTY_FUNCTION__), level)

#define INTERNAL_CONTRACT_MESSAGE(boolean_expression)                          \
  LogCapture(__FILE__, __LINE__, __PRETTY_FUNCTION__, g3::internal::CONTRACT,  \
             boolean_expression)

// GLOG_LOG(level) is the API for the stream log
#define GLOG_LOG(level)                                                        \
  if (!g3::logLevel(level)) {                                                  \
  } else                                                                       \
    INTERNAL_LOG_MESSAGE(level).stream()

// 'Conditional' stream log
#define GLOG_LOG_IF(level, boolean_expression)                                 \
  if (true == (boolean_expression))                                            \
    if (g3::logLevel(level))                                                   \
  INTERNAL_LOG_MESSAGE(level).stream()

// Use marco expansion to create, for each use of GLOG_EVERY_N(), static
// variables with the __LINE__ expansion as part of the variable name.
#define LOG_EVERY_N_VARNAME(base, line) LOG_EVERY_N_VARNAME_CONCAT(base, line)
#define LOG_EVERY_N_VARNAME_CONCAT(base, line) base##line
#define LOG_OCCURRENCES LOG_EVERY_N_VARNAME(occurrences_, __LINE__)
#define LOG_OCCURRENCES_MOD_N LOG_EVERY_N_VARNAME(occurrences_mod_n_, __LINE__)

#define GLOG_LOG_EVERY_N(level, n)                                             \
  static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0;                   \
  ++LOG_OCCURRENCES;                                                           \
  if (++LOG_OCCURRENCES_MOD_N > n)                                             \
    LOG_OCCURRENCES_MOD_N -= n;                                                \
  if (LOG_OCCURRENCES_MOD_N == 1)                                              \
    if (g3::logLevel(level))                                                   \
  INTERNAL_LOG_MESSAGE(level).stream()

// VLOG support
#define GLOG_VLOG(verboselevel)                                                \
  GLOG_LOG_IF(G3LOG_INFO, FLAGS_v >= (verboselevel))

// 'Design By Contract' stream API. For Broken Contracts:
//         unit testing: it will throw std::runtime_error when a contract breaks
//         I.R.L : it will exit the application by using fatal signal SIGABRT
#define CHECK(boolean_expression)                                              \
  if (!(boolean_expression))                                                   \
  INTERNAL_CONTRACT_MESSAGE(#boolean_expression).stream()

// naive CHECK_OP implement
#define CHECK_EQ(val1, val2) CHECK((val1) == (val2))
#define CHECK_NE(val1, val2) CHECK((val1) != (val2))
#define CHECK_LE(val1, val2) CHECK((val1) <= (val2))
#define CHECK_LT(val1, val2) CHECK((val1) < (val2))
#define CHECK_GE(val1, val2) CHECK((val1) >= (val2))
#define CHECK_GT(val1, val2) CHECK((val1) > (val2))
#define CHECK_NOTNULL(val)                                                     \
  CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// support for DCHECK
#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#define DCHECK_IS_ON() 0
#else
#define DCHECK_IS_ON() 1
#endif

#if DCHECK_IS_ON()
#define DLOG(level) GLOG_LOG(level)
#define DVLOG(verboselevel) GLOG_VLOG(verboselevel)
#define DLOG_IF(level, condition) GLOG_LOG_IF(level, condition)
#define DCHECK(condition) CHECK(condition)
#define DCHECK_EQ(val1, val2) CHECK_EQ(val1, val2)
#define DCHECK_NE(val1, val2) CHECK_NE(val1, val2)
#define DCHECK_LE(val1, val2) CHECK_LE(val1, val2)
#define DCHECK_LT(val1, val2) CHECK_LT(val1, val2)
#define DCHECK_GE(val1, val2) CHECK_GE(val1, val2)
#define DCHECK_GT(val1, val2) CHECK_GT(val1, val2)
#define DCHECK_NOTNULL(val) CHECK_NOTNULL(val)
#else
// should use while instead of if
#define DLOG(level)                                                            \
  if constexpr (false)                                                         \
  GLOG_LOG(level)
#define DVLOG(verboselevel)                                                    \
  if constexpr (false)                                                         \
  GLOG_VLOG(verboselevel)
#define DLOG_IF(level, condition)                                              \
  if constexpr (false)                                                         \
  GLOG_LOG_IF(level, condition)
#define DCHECK(condition)                                                      \
  if constexpr (false)                                                         \
  CHECK(condition)
#define DCHECK_EQ(val1, val2)                                                  \
  if constexpr (false)                                                         \
  CHECK_EQ(val1, val2)
#define DCHECK_NE(val1, val2)                                                  \
  if constexpr (false)                                                         \
  CHECK_NE(val1, val2)
#define DCHECK_LE(val1, val2)                                                  \
  if constexpr (false)                                                         \
  CHECK_LE(val1, val2)
#define DCHECK_LT(val1, val2)                                                  \
  if constexpr (false)                                                         \
  CHECK_LT(val1, val2)
#define DCHECK_GE(val1, val2)                                                  \
  if constexpr (false)                                                         \
  CHECK_GE(val1, val2)
#define DCHECK_GT(val1, val2)                                                  \
  if constexpr (false)                                                         \
  CHECK_GT(val1, val2)
#define DCHECK_NOTNULL(val)                                                    \
  if constexpr (false)                                                         \
  CHECK_NOTNULL(val)
#endif

/** For details please see this
 * REFERENCE: http://www.cppreference.com/wiki/io/c/printf_format
 * \verbatim
 *
  There are different %-codes for different variable types, as well as options
to limit the length of the variables and whatnot. Code Format
    %[flags][width][.precision][length]specifier
 SPECIFIERS
 ----------
 %c character
 %d signed integers
 %i signed integers
 %e scientific notation, with a lowercase “e”
 %E scientific notation, with a uppercase “E”
 %f floating point
 %g use %e or %f, whichever is shorter
 %G use %E or %f, whichever is shorter
 %o octal
 %s a string of characters
 %u unsigned integer
 %x unsigned hexadecimal, with lowercase letters
 %X unsigned hexadecimal, with uppercase letters
 %p a pointer
 %n the argument shall be a pointer to an integer into which is placed the
number of characters written so far

For flags, width, precision etc please see the above references.
EXAMPLES:
{
   GLOG_LOGF(G3LOG_INFO, "Characters: %c %c \n", 'a', 65);
   GLOG_LOGF(G3LOG_INFO, "Decimals: %d %ld\n", 1977, 650000L);      // printing
long GLOG_LOGF(G3LOG_INFO, "Preceding with blanks: %10d \n", 1977);
   GLOG_LOGF(G3LOG_INFO, "Preceding with zeros: %010d \n", 1977);
   GLOG_LOGF(G3LOG_INFO, "Some different radixes: %d %x %o %#x %#o \n", 100,
100, 100, 100, 100); GLOG_LOGF(G3LOG_INFO, "floats: %4.2f %+.0e %E
\n", 3.1416, 3.1416, 3.1416); GLOG_LOGF(G3LOG_INFO, "Width trick: %*d \n", 5,
10); GLOG_LOGF(G3LOG_INFO, "%s \n", "A string"); return 0;
}
And here is possible output
:      Characters: a A
:      Decimals: 1977 650000
:      Preceding with blanks:       1977
:      Preceding with zeros: 0000001977
:      Some different radixes: 100 64 144 0x64 0144
:      floats: 3.14 +3e+000 3.141600E+000
:      Width trick:    10
:      A string  \endverbatim */
#define GLOG_LOGF(level, printf_like_message, ...)                             \
  if (!g3::logLevel(level)) {                                                  \
  } else                                                                       \
    INTERNAL_LOG_MESSAGE(level).capturef(printf_like_message, ##__VA_ARGS__)

// Conditional log printf syntax
#define GLOG_LOGF_IF(level, boolean_expression, printf_like_message, ...)      \
  if (true == (boolean_expression))                                            \
    if (g3::logLevel(level))                                                   \
  INTERNAL_LOG_MESSAGE(level).capturef(printf_like_message, ##__VA_ARGS__)

// Design By Contract, printf-like API syntax with variadic input parameters.
// Throws std::runtime_eror if contract breaks
#define CHECKF(boolean_expression, printf_like_message, ...)                   \
  if (false == (boolean_expression))                                           \
  INTERNAL_CONTRACT_MESSAGE(#boolean_expression)                               \
      .capturef(printf_like_message, ##__VA_ARGS__)

// Backwards compatible. The same as CHECKF.
// Design By Contract, printf-like API syntax with variadic input parameters.
// Throws std::runtime_eror if contract breaks
#define CHECK_F(boolean_expression, printf_like_message, ...)                  \
  if (false == (boolean_expression))                                           \
  INTERNAL_CONTRACT_MESSAGE(#boolean_expression)                               \
      .capturef(printf_like_message, ##__VA_ARGS__)

// Check if it's compiled in C++11 mode.
//
// GXX_EXPERIMENTAL_CXX0X is defined by gcc and clang up to at least
// gcc-4.7 and clang-3.1 (2011-12-13).  __cplusplus was defined to 1
// in gcc before 4.7 (Crosstool 16) and clang before 3.1, but is
// defined according to the language version in effect thereafter.
// Microsoft Visual Studio 14 (2015) sets __cplusplus==199711 despite
// reasonably good C++11 support, so we set LANG_CXX for it and
// newer versions (_MSC_VER >= 1900).
#if (defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L ||          \
     (defined(_MSC_VER) && _MSC_VER >= 1900))
// Helper for CHECK_NOTNULL().
//
// In C++11, all cases can be handled by a single function. Since the value
// category of the argument is preserved (also for rvalue references),
// member initializer lists like the one below will compile correctly:
//
//   Foo()
//     : x_(CHECK_NOTNULL(MethodReturningUniquePtr())) {}
template <typename T>
T CheckNotNull(const char *file, int line, const char *names, T &&t) {
  if (t == nullptr) {
    INTERNAL_CONTRACT_MESSAGE(names);
  }
  return std::forward<T>(t);
}

#else

// A small helper for CHECK_NOTNULL().
template <typename T>
T *CheckNotNull(const char *file, int line, const char *names, T *t) {
  if (t == NULL) {
    INTERNAL_CONTRACT_MESSAGE(names);
  }
  return t;
}
#endif
