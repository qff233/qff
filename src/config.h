#ifndef __QFF_CONFIG_H__
#define __QFF_CONFIG_H__

#include <memory>
#include <variant>
#include "yaml-cpp/yaml.h"
#include "singleton.h"
#include "macro.h"

namespace qff {

class ConfigVar {
public:
    NONECOPYABLE(ConfigVar);
    using ptr = std::shared_ptr<ConfigVar>;

    enum Type {
        INT      = 0,
        STRING   = 1,
        List     = 2
    };

    template<typename T>
    ConfigVar(const std::string& name, const T& val, const std::string& despription = "") 
        :m_name(name)
        ,m_val(m_val)
        ,m_despription(despription) {
    }

    const std::string& get_name() const { return m_name; }
    const std::string& get_desprition() const { return m_despription; }
private:
    std::string m_name;
    std::string m_despription;
    std::variant<int, std::string> m_val;
    std::vector<ConfigVar::ptr> m_vars;
};

class ConfigManager {
public:
    NONECOPYABLE(ConfigManager);

    void load_config_from_path(const std::string& path);
    void load_config_from_file(const std::string& file_name);

    void add_config(ConfigVar::ptr config_var);
    template<typename T>
    void add_config(const std::string& name, const T& val
            , const std::string& despription="") {
        ConfigVar::ptr config_var = std::make_shared<ConfigVar>(name, val, despription);
        add_config(config_var);
    }

    ConfigVar::ptr get_config(const std::string& name);
    template<typename T>
    ConfigVar::ptr get_config(const std::string& name, const T& default_val
            , const std::string& despription="") {
        auto p_config = this->get_config(name);
        if(p_config != nullptr)
            return p_config;
        ConfigVar::ptr config_var = std::make_shared<ConfigVar>(name, default_val, despription);
        add_config(config_var);
        return config_var;
    }
private:
    std::vector<ConfigVar::ptr> m_datas;
};

using ConfigMgr = Singleton<ConfigManager>;

}

#endif