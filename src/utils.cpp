#include "utils.h"

#include <algorithm>
#include <sstream>
#include <fstream>
#include <cstring>
#include <execinfo.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <signal.h>
#include <cxxabi.h>
#include <dirent.h>

#include "log.h"
#include "thread.h"

namespace qff {

pid_t GetThreadId() noexcept {
    return syscall(SYS_gettid);
}

fid_t GetFiberId() noexcept {
    return qff::Fiber::GetFiberId();
}

std::string GetThreadName() {
    return Thread::GetName();
}

time_t GetCurrentMS() noexcept {
    struct timeval tv;
    ::gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000ul  + tv.tv_usec / 1000;
}

time_t GetCurrentUS() noexcept {
    struct timeval tv;
    ::gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000ul  + tv.tv_usec;
}

namespace StringUtils {

std::string StringToUpper(std::string_view str) noexcept {
    std::string rt;
    std::transform(str.begin(), str.end(), rt, ::toupper);
    return rt;
}

std::string StringToLower(std::string_view str) noexcept {
    std::string rt;
    std::transform(str.begin(), str.end(), rt, ::tolower);
    return rt;
}

std::vector<std::string> Spilt(std::string_view str, char sign) {
    std::vector<std::string> sub_strs;
    size_t now_pos = 0;
    size_t dot_pos = std::string::npos;
    std::string sub_str;
    for(;;) {
        dot_pos = str.find(sign, now_pos);
        if(dot_pos == std::string::npos) {
            sub_str = 
                str.substr(now_pos, str.size() - now_pos);
            sub_strs.emplace_back(sub_str);
            break;
        }
        sub_str = str.substr(now_pos, dot_pos - now_pos);
        sub_strs.emplace_back(sub_str);
        now_pos = dot_pos + 1;
    }
    return sub_strs;
}

static std::string __formatv(const char* fmt, va_list ap) {
    char* buf = nullptr;
    auto len = ::vasprintf(&buf, fmt, ap);
    if(len == -1) {
        return "";
    }
    std::string ret(buf, len);
    ::free(buf);
    return ret;
}

std::string Format(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    auto v = __formatv(fmt, ap);
    va_end(ap);
    return v;
}

}//namesapce StringUtils

namespace FSUtils {

static void __list_all_file(std::vector<std::string>& files
                        ,const std::string& path) {
    if(::access(path.data(), 0) != 0) {
        return;
    }
    DIR* dir = ::opendir(path.data());
    if(dir == nullptr) {
        return;
    }
    struct dirent* dp = nullptr;
    
    while((dp = ::readdir(dir)) != nullptr) {
        if(dp->d_type == DT_DIR) {
            if(!strcmp(dp->d_name, ".")
                || !strcmp(dp->d_name, "..")) {
                continue;
            }
            __list_all_file(files, path + "/" + dp->d_name);
        } else if(dp->d_type == DT_REG) {
            std::string file_name(dp->d_name);
            files.push_back(path + "/" + file_name);
        }
    }
    ::closedir(dir);
}

std::vector<std::string> ListAllFile(std::string_view path) {
    std::vector<std::string> files;
    __list_all_file(files, path.data());
    return files;
}

static int __lstat(const char* file, struct stat* st = nullptr) noexcept {
    struct stat lst;
    int ret = ::lstat(file, &lst);
    if(st) {
        *st = lst;
    }
    return ret;
}

static int __mkdir(const char* dirname) noexcept {
    if(::access(dirname, F_OK) == 0) {
        return 0;
    }
    return ::mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

bool MkDir(std::string_view dirname) noexcept {
    if(__lstat(dirname.data()) == 0) {
        return true;
    }
    char* path = ::strdup(dirname.data());
    char* ptr = ::strchr(path + 1, '/');
    do {
        for(; ptr; *ptr = '/', ptr = ::strchr(ptr + 1, '/')) {
            *ptr = '\0';
            if(__mkdir(path) != 0) {
                break;
            }
        }
        if(ptr != nullptr) {
            break;
        } else if(__mkdir(path) != 0) {
            break;
        }
        ::free(path);
        return true;
    } while(0);
    ::free(path);
    return false;
}

bool IsRunningPidFile(std::string_view pidfile) {
    if(__lstat(pidfile.data()) != 0) {
        return false;
    }
    std::ifstream ifs(pidfile.data());
    std::string line;
    if(!ifs || !std::getline(ifs, line)) {
        return false;
    }
    if(line.empty()) {
        return false;
    }
    pid_t pid = atoi(line.c_str());
    if(pid <= 1) {
        return false;
    }
    if(::kill(pid, 0) != 0) {
        return false;
    }
    return true;
}

bool UnLink(std::string_view filename, bool exist) noexcept {
    if(!exist && __lstat(filename.data())) {
        return true;
    }
    return ::unlink(filename.data()) == 0;
}

bool Rm(std::string_view path) {
    struct stat st;
    if(::lstat(path.data(), &st)) {
        return true;
    }
    if(!(st.st_mode & S_IFDIR)) {
        return UnLink(path);
    }

    DIR* dir = ::opendir(path.data());
    if(!dir) {
        return false;
    }
    
    bool ret = true;
    struct dirent* dp = nullptr;
    while((dp = ::readdir(dir))) {
        if(!strcmp(dp->d_name, ".")
                || !strcmp(dp->d_name, "..")) {
            continue;
        }
        std::string dirname = path.data();
        dirname += "/";
        dirname += dp->d_name;
        ret = Rm(dirname);
    }
    ::closedir(dir);
    if(::rmdir(path.data())) {
        ret = false;
    }
    return ret;
}

bool Mv(std::string_view from, std::string_view to) {
    if(!Rm(to)) {
        return false;
    }
    return rename(from.data(), to.data()) == 0;
}

bool RealPath(std::string_view path, std::string& rpath) noexcept {
    if(__lstat(path.data())) {
        return false;
    }
    char* ptr = ::realpath(path.data(), nullptr);
    if(nullptr == ptr) {
        return false;
    }
    std::string(ptr).swap(rpath);
    ::free(ptr);
    return true;
}

bool SymLink(std::string_view from, std::string_view to) noexcept {
    if(!Rm(to)) {
        return false;
    }
    return ::symlink(from.data(), to.data()) == 0;
}

std::string DirName(std::string_view filename) noexcept {
    if(filename.empty()) {
        return ".";
    }
    auto pos = filename.rfind('/');
    if(pos == 0) {
        return "/";
    } else if(pos == std::string::npos) {
        return ".";
    } else {
        std::string rt = filename.substr(0, pos).data();
        return rt;
    }
}

std::string BaseName(std::string_view filename) noexcept {
    if(filename.empty()) {
        return filename.data();
    }
    auto pos = filename.rfind('/');
    if(pos == std::string::npos) {
        return filename.data();
    } else {
        return filename.substr(pos + 1).data();
    }
}

bool OpenForRead(std::ifstream& ifs, std::string_view filename
                        ,std::ios_base::openmode mode) {
    ifs.open(filename.data(), mode);
    return ifs.is_open();
}

bool OpenForWrite(std::ofstream& ofs, std::string_view filename
                        ,std::ios_base::openmode mode) {
    ofs.open(filename.data(), mode);   
    if(!ofs.is_open()) {
        std::string dir = DirName(filename);
        MkDir(dir);
        ofs.open(filename.data(), mode);
    }
    return ofs.is_open();
}


} //FSUtils

std::string Time2Str(time_t ts, std::string_view format) noexcept {
    struct tm tm;
    localtime_r(&ts, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), format.data(), &tm);
    return buf;
}

time_t Str2Time(const char* str, const char* format) noexcept {
    struct tm t;
    memset(&t, 0, sizeof(t));
    if(!strptime(str, format, &t)) {
        return 0;
    }
    return mktime(&t);
}

YAML::Node GetYamlFromFile(std::string_view file_name) {
    YAML::Node config;
    try {
        config = YAML::LoadFile(file_name.data());
    } catch(const YAML::Exception& e) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) 
            << "yaml: LoadFile is error. there is a rensen that   " << e.what();
        return YAML::Node();
        throw;
    }
    return config;
}

}