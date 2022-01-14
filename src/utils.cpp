#include "utils.h"

#include <algorithm>
#include <sstream>
#include <cstring>
#include <execinfo.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <cxxabi.h>

#include "log.h"

namespace qff {

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint32_t GetFiberId() {
    return 0;
}


time_t GetCurrentMS() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000ul  + tv.tv_usec / 1000;
}

time_t GetCurrentUS() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000ul  + tv.tv_usec;
}

std::string ToUpper(const std::string& str) {
    std::string rt = str;
    std::transform(rt.begin(), rt.end(), rt.begin(), ::toupper);
    return rt;
}

std::string ToLower(const std::string& str) {
    std::string rt = str;
    std::transform(rt.begin(), rt.end(), rt.begin(), ::tolower);
    return rt;
}

std::string Time2Str(time_t ts, const std::string& format) {
    struct tm tm;
    localtime_r(&ts, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), format.c_str(), &tm);
    return buf;
}

time_t Str2Time(const char* str, const char* format) {
    struct tm t;
    memset(&t, 0, sizeof(t));
    if(!strptime(str, format, &t)) {
        return 0;
    }
    return mktime(&t);
}


}