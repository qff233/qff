#ifndef __QFF_CONFIG_H__
#define __QFF_CONFIG_H__

#include <memory>
#include <variant>
#include "yaml-cpp/yaml.h"
#include "singleton.h"
#include "macro.h"
#include "thread.h"

namespace qff {

struct ConfigVar {
    typedef std::shared_ptr<ConfigVar> ptr;

    enum Type {
        NONE   = 0,
        STRING = 1,
        LIST   = 2
    };

    std::string key;
    std::string val;
    Type type;
    std::vector<ConfigVar::ptr> nexts;

    ConfigVar();
    ConfigVar(const std::string& key);
    ConfigVar(const std::string& key, const std::string& value);
}; //struct ConfigVar

class Config {
friend std::ostream& operator<<(std::ostream& os, const Config& config);
public:
    NONECOPYABLE(Config);
    typedef  ConfigVar NodeType;
    typedef  std::vector<ConfigVar::ptr> NodesType;
    
    
    Config();

    void load_from_path(const std::string& path);
    void load_from_file(const std::string& file_name);
    
    int get_int_var(const std::string& key) noexcept;
    std::string get_string_var(const std::string& key) noexcept;
    std::vector<std::string> get_vec_string_var(const std::string& key);
    std::vector<int> get_vec_int_var(const std::string& key);
private:
    NodesType m_roots;
}; //class Config

std::ostream& operator<<(std::ostream& os, const Config& config);

using ConfigMgr = Singleton<Config>;

} //namespace qff

#endif