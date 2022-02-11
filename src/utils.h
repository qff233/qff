#ifndef __QFF_UTILS_H__
#define __QFF_UTILS_H__

#include <string>
#include <vector>
#include <string_view>
#include "yaml-cpp/yaml.h"
#include "fiber.h"

namespace qff {

pid_t GetThreadId() noexcept;
std::string GetThreadName();

fid_t GetFiberId() noexcept;

time_t GetCurrentMS() noexcept;
time_t GetCurrentUS() noexcept;

namespace StringUtils {
    std::string StringToUpper(std::string_view str) noexcept;
    std::string StringToLower(std::string_view str) noexcept;

    std::vector<std::string> Spilt(std::string_view str, char sign);

    std::string Format(const char* fmt, ...);
}

namespace FSUtils {
    std::vector<std::string> ListAllFile(std::string_view path);
    bool MkDir(std::string_view dirname) noexcept;
    bool IsRunningPidFile(std::string_view pidfile);
    bool Rm(std::string_view path);
    bool Mv(std::string_view from, std::string_view to);
    bool RealPath(std::string_view path, std::string& rpath) noexcept;
    bool SymLink(std::string_view frm, std::string_view to) noexcept;
    bool UnLink(std::string_view filename, bool exist = false) noexcept;
    std::string DirName(std::string_view filename) noexcept;
    std::string BaseName(std::string_view filename) noexcept;
    bool OpenForRead(std::ifstream& ifs, std::string_view filename
                    ,std::ios_base::openmode mode);
    bool OpenForWrite(std::ofstream& ofs, std::string_view filename
                    ,std::ios_base::openmode mode);
}

std::string Time2Str(time_t ts = time(0), std::string_view format = "%Y-%m-%d %H:%M:%S") noexcept;
time_t Str2Time(const char* str, const char* format = "%Y-%m-%d %H:%M:%S") noexcept;

YAML::Node GetYamlFromFile(std::string_view file_name);

}

#endif