#include "g3log/time.hpp"
#include "g3log/g3log.hpp"
#include <gtest/gtest.h>
#include <cstdio>

namespace {
  auto check = [](auto a, auto b, const char *msg = "", bool print = false) {
    if (a == b) {
      if (print) {
        std::cout << "[pass] " << msg << ": " << a << std::endl;
      }
    }
    else {
      std::cerr << "[failed] " << msg << ": Expected: " << a << ", got: " << b << std::endl;
    }
  };
}

TEST(LocalTime, Before2017) {
  using namespace g3;
  std::time_t kTime20160509 = 1462752000;
  struct tm a = localtime(kTime20160509);
  struct tm b = mylocaltime(kTime20160509);
  EXPECT_EQ(a.tm_year, b.tm_year);
  EXPECT_EQ(a.tm_mon, b.tm_mon);
  EXPECT_EQ(a.tm_hour, b.tm_hour);
  EXPECT_EQ(a.tm_min, b.tm_min);
  EXPECT_EQ(a.tm_mday, b.tm_mday);
  EXPECT_EQ(a.tm_yday, b.tm_yday);
  EXPECT_EQ(a.tm_wday, b.tm_wday);
  EXPECT_EQ(a.tm_isdst, b.tm_isdst);
}

TEST(LocalTime, After2017) {
  using namespace g3;
  std::time_t kTimes[] = {1525838601, 1541736201, 1646831016};
  for (auto ts : kTimes) {
    struct tm a = localtime(ts);
    struct tm b = mylocaltime(ts);
    EXPECT_EQ(a.tm_year, b.tm_year) << "Suppose: " << a.tm_year << " Got: " << b.tm_year;
    EXPECT_EQ(a.tm_mon, b.tm_mon) << "Suppose: " << a.tm_mon << " Got: " << b.tm_mon;
    EXPECT_EQ(a.tm_hour, b.tm_hour) << "Suppose: " << a.tm_hour << " Got: " << b.tm_hour;
    EXPECT_EQ(a.tm_min, b.tm_min) << "Suppose: " << a.tm_min << " Got: " << b.tm_min;
    EXPECT_EQ(a.tm_mday, b.tm_mday) << "Suppose: " << a.tm_mday << " Got: " << b.tm_mday;
    EXPECT_EQ(a.tm_yday, b.tm_yday) << "Suppose: " << a.tm_yday << " Got: " << b.tm_yday;
    EXPECT_EQ(a.tm_wday, b.tm_wday) << "Suppose: " << a.tm_wday << " Got: " << b.tm_wday;
    EXPECT_EQ(a.tm_isdst, b.tm_isdst) << "Suppose: " << a.tm_isdst << " Got: " << b.tm_isdst;
  }
}

int main(int argc, char *argv[])
{
   testing::InitGoogleTest(&argc, argv);
   int return_value = RUN_ALL_TESTS();
   std::cout << "FINISHED WITH THE TESTING" << std::endl;
   return return_value;
}
