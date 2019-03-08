/** ==========================================================================
* 2012 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
* with no warranties. This code is yours to share, use and modify with no
* strings attached and no restrictions or obligations.
*
* For more information see g3log/LICENSE or refer refer to http://unlicense.org
* ============================================================================*/

#include "g3log/time.hpp"

#include <sstream>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>
#include <cassert>
#include <iomanip>

#include <sys/time.h>
#ifdef __MACH__
#include <sys/time.h>
#endif

namespace g3 {
   namespace internal {
      const std::string kFractionalIdentier   = "%f";
      const size_t kFractionalIdentierSize = 2;

      Fractional getFractional(const std::string& format_buffer, size_t pos) {
         char  ch  = (format_buffer.size() > pos + kFractionalIdentierSize ? format_buffer.at(pos + kFractionalIdentierSize) : '\0');
         Fractional type = Fractional::NanosecondDefault;
         switch (ch) {
            case '3': type = Fractional::Millisecond; break;
            case '6': type = Fractional::Microsecond; break;
            case '9': type = Fractional::Nanosecond; break;
            default: type = Fractional::NanosecondDefault; break;
         }
         return type;
      }

      // Returns the fractional as a string with padded zeroes
      // 1 ms --> 001
      // 1 us --> 000001
      // 1 ns --> 000000001
      std::string to_string(const g3::system_time_point& ts, Fractional fractional) {
         auto duration = ts.time_since_epoch();
         auto sec_duration = std::chrono::duration_cast<std::chrono::seconds>(duration);
         duration -= sec_duration;
         auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

         auto zeroes = 9; // default ns
         auto digitsToCut = 1; // default ns, divide by 1 makes no change
         switch (fractional) {
            case Fractional::Millisecond : {
               zeroes = 3;
               digitsToCut = 1000000;
               break;
            }
            case Fractional::Microsecond : {
               zeroes = 6;
               digitsToCut = 1000;
               break;
            }
            case Fractional::Nanosecond :
            case Fractional::NanosecondDefault:
            default:
               zeroes = 9;
               digitsToCut = 1;

         }

         ns /= digitsToCut;
         auto value = std::string(std::to_string(ns));
         return std::string(zeroes - value.size(), '0') + value;
      }

      std::string localtime_formatted_fractions(const g3::system_time_point& ts, std::string format_buffer) {
         // iterating through every "%f" instance in the format string
         auto identifierExtraSize = 0;
         for (size_t pos = 0;
               (pos = format_buffer.find(g3::internal::kFractionalIdentier, pos)) != std::string::npos;
               pos += g3::internal::kFractionalIdentierSize + identifierExtraSize) {
            // figuring out whether this is nano, micro or milli identifier
            auto type = g3::internal::getFractional(format_buffer, pos);
            auto value = g3::internal::to_string(ts, type);
            auto padding = 0;
            if (type != g3::internal::Fractional::NanosecondDefault) {
               padding = 1;
            }

            // replacing "%f[3|6|9]" with sec fractional part value
            format_buffer.replace(pos, g3::internal::kFractionalIdentier.size() + padding, value);
         }
         return format_buffer;
      }

   } // internal
} // g3



namespace g3 {
   // This mimics the original "std::put_time(const std::tm* tmb, const charT* fmt)"
   // This is needed since latest version (at time of writing) of gcc4.7 does not implement this library function yet.
   // return value is SIMPLIFIED to only return a std::string
   std::string put_time(const struct tm* tmb, const char* c_time_format) {
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__)) && !defined(__MINGW32__)
      std::ostringstream oss;
      oss.fill('0');
      // BOGUS hack done for VS2012: C++11 non-conformant since it SHOULD take a "const struct tm*  "
      oss << std::put_time(const_cast<struct tm*> (tmb), c_time_format);
      return oss.str();
#else    // LINUX
      const size_t size = 1024;
      char buffer[size]; // IMPORTANT: check now and then for when gcc will implement std::put_time.
      //                    ... also ... This is way more buffer space then we need

      auto success = std::strftime(buffer, size, c_time_format, tmb);
      // In DEBUG the assert will trigger a process exit. Once inside the if-statement
      // the 'always true' expression will be displayed as reason for the exit
      //
      // In Production mode
      // the assert will do nothing but the format string will instead be returned
      if (0 == success) {
         assert((0 != success) && "strftime fails with illegal formatting");
         return c_time_format;
      }
      return buffer;
#endif
   }



   tm localtime(const std::time_t& ts) {
      struct tm tm_snapshot;
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
      localtime_s(&tm_snapshot, &ts); // windsows
#else
      localtime_r(&ts, &tm_snapshot); // POSIX
#endif
      return tm_snapshot;
   }

   // change to gettimeofday to speed up this function
   tm mylocaltime(const std::time_t& ts) {
      struct tm tm_snapshot;
      
      // we asume the time stamp no eariler than 2017.01.01 Sunday(GMT)
      const std::time_t base_time = 1483228800;
      const int base_year = 2017;
      
      // define const to speed up calculation
      const int sec_per_day = 86400;
      const int sec_per_4year = sec_per_day * (365 * 4 + 1);
      // sec_per_year[1] for leap year
      const int sec_per_year[] = {sec_per_day * 365, sec_per_day * 366};
      const int day_per_month[] = {31,28,31,30,31, 30,31,31,30,31, 30,31};
      const int day_per_month_leapyear[] = {31,29,31,30,31, 30,31,31,30,31, 30,31};

      // add time zone info here
      // process time zone info
      struct timeval tv;
      struct timezone tz;
      gettimeofday(&tv, &tz);

      

      // time which is eariler than 2017
      if (ts <= base_time) {
         return localtime(ts);
      }

      // total time left
      long secs = static_cast<long>(ts - base_time);
      // See: http://man7.org/linux/man-pages/man2/gettimeofday.2.html
      // TODO tz field is obsolete, and will always return ZERO
      const int kTimeZone = 8;
      secs += kTimeZone * 3600;
      
      // determine leap year
      auto isLeap = [](int year) {
         return (year % 400 == 0) || (year % 100 != 0 && year % 4 == 0);
      };

      // day since Sunday
      int wday = secs / sec_per_day % 7;  // since 2017.01.01 (Sunday)

      int year  = base_year;
      while(secs > sec_per_year[isLeap(year)]){
         secs -= sec_per_year[isLeap(year)];
         year++;
      }

      // day since January 1st: 0-365
      int yday = secs / sec_per_day;

      int month = 0;
      if (isLeap(year)) {
         while(secs > day_per_month_leapyear[month] * sec_per_day){
            secs -= day_per_month_leapyear[month] * sec_per_day;
            month++;
         }
      } else {
         while(secs > day_per_month[month] * sec_per_day){
            secs -= day_per_month[month] * sec_per_day;
            month++;
         }
      }

      int mday = secs / sec_per_day;  // day of the month: range 1-31
      secs -= mday * sec_per_day;
      int hour = secs / 3600; 
      secs -= hour * 3600;
      int minutes = secs / 60;
      secs = secs % 60;

      
      
      tm_snapshot.tm_year = year - 1900;  // year from 1900
      tm_snapshot.tm_mon = month;  // range: 0-11
      tm_snapshot.tm_mday = mday + 1;   // range: 1-31
      tm_snapshot.tm_yday = yday;   // range: 0-365
      tm_snapshot.tm_wday = wday;   // range: 0-6 
      tm_snapshot.tm_hour = hour;   // range: 0-23
      tm_snapshot.tm_min = minutes;    // range:0-59
      tm_snapshot.tm_sec = secs; 
      tm_snapshot.tm_zone = "GTM+8";  // unfinished
      tm_snapshot.tm_isdst = tz.tz_dsttime;  
      
      return tm_snapshot;
   }

   std::string localtime_formatted(const g3::system_time_point& ts, const std::string& time_format) {
      auto format_buffer = internal::localtime_formatted_fractions(ts, time_format);
      auto time_point = std::chrono::system_clock::to_time_t(ts);
      std::tm t = mylocaltime(time_point);
      return g3::put_time(&t, format_buffer.c_str()); // format example: //"%Y/%m/%d %H:%M:%S");
   }
} // g3
