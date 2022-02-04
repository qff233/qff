#ifndef __QFF_UTILS_H__
#define __QFF_UTILS_H__

#include <string>
#include <vector>
#include "yaml-cpp/yaml.h"
#include "fiber.h"

namespace qff {

pid_t GetThreadId();
std::string GetThreadName();

fid_t GetFiberId();

time_t GetCurrentMS();
time_t GetCurrentUS();

namespace StringUtils {
    std::string StringToUpper(const std::string& str);
    std::string StringToLower(const std::string& str);

    std::vector<std::string> Spilt(const std::string& str, char sign);

    std::string Format(const char* fmt, ...);
}

namespace FSUtils {
    std::vector<std::string> ListAllFile(const std::string& path);
    bool MkDir(const std::string& dirname);
    bool IsRunningPidFile(const std::string& pidfile);
    bool Rm(const std::string& path);
    bool Mv(const std::string& from, const std::string& to);
    bool RealPath(const std::string& path, std::string& rpath);
    bool SymLink(const std::string& frm, const std::string& to);
    bool UnLink(const std::string& filename, bool exist = false);
    std::string DirName(const std::string& filename);
    std::string BaseName(const std::string& filename);
    bool OpenForRead(std::ifstream& ifs, const std::string& filename
                    ,std::ios_base::openmode mode);
    bool OpenForWrite(std::ofstream& ofs, const std::string& filename
                    ,std::ios_base::openmode mode);
}

std::string Time2Str(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");
time_t Str2Time(const char* str, const char* format = "%Y-%m-%d %H:%M:%S");

YAML::Node GetYamlFromFile(const std::string& file_name);

}

#endif