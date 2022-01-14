#ifndef __QFF_UTILS_H__
#define __QFF_UTILS_H__

#include <string>
#include <vector>

namespace qff {

pid_t GetThreadId();
uint32_t GetFiberId();

time_t GetCurrentMS();
time_t GetCurrentUS();

std::string ToUpper(const std::string& str);
std::string ToLower(const std::string& str);

std::string Time2Str(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");
time_t Str2Time(const char* str, const char* format = "%Y-%m-%d %H:%M:%S");

}

#endif